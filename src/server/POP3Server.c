#include "POP3Server.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "selector.h"

#include <sys/socket.h>

#include "buffer.h"
#include "stm.h"
#include "pop3Parser.h"
#include "greetings.h"
#include "auth.h"
#include "transaction.h"
#include "update.h"
#include "selector.h"

#define BUFFER_SIZE 8192

#define SUCCESS_MSG "+OK "
#define ERROR_MSG "-ERR "


char* mailDirectory = NULL;

//-------------------------------------Generic handler (just to support pipelining)-------------------------
static unsigned readOnReady(struct selector_key * key) {
    clientData* data = ATTACHMENT(key);
    bool isFinished = readAndParse(key);
    if (isFinished) {
        printf("Entrando a read finished\n");
        unsigned next;
        switch (stm_state(&data->stateMachine)) {
        case AUTHORIZATION:
            next = authOnReadReady(key);
            break;
        case TRANSACTION:
            next= transactionOnReadReady(key);
            break;
        }
        resetParser(&data->pop3Parser);
        selector_set_interest_key(key, OP_WRITE);
        return next;
    }
    return stm_state(&data->stateMachine);
}

static unsigned writeOnReady(struct selector_key * key) {
    clientData* data = ATTACHMENT(key);
    bool isFinished = sendFromBuffer(key);
    if (isFinished) {
        unsigned next;
        switch (stm_state(&data->stateMachine)) {
        case GREETINGS:
            next = AUTHORIZATION;
            break;
        case AUTHORIZATION:
            if (data->isAuth)
                next = TRANSACTION;
            else
                next = AUTHORIZATION;
            break;
        case TRANSACTION:
            next = TRANSACTION;
            break;
        case UPDATE:
            next = DONE;
        }

        if (!buffer_can_read(&data->pop3Parser.buffer)) {
            printf("no quedo nada y retorno %d", next);
            selector_set_interest_key(key, OP_READ);
            return next;
        }

        printf("queda algo en el buffer y salto a %d", next);
        jump(&data->stateMachine, next, key);
        selector_set_interest_key(key, OP_READ);
        readOnReady(key);
    }
    return stm_state(&data->stateMachine);
}

//-------------------------------------Array de estados para la stm------------------------------------------
static const struct state_definition stateHandlers[] = {
    {
        .state = GREETINGS,
        .on_arrival = greetingOnArrival,
        .on_write_ready = writeOnReady
    },
    {
        .state = AUTHORIZATION,
        .on_arrival = authOnArrival,
        .on_read_ready = readOnReady,
        .on_write_ready = writeOnReady,
    },
    {
        .state = TRANSACTION,
        .on_arrival = transactionOnArrival,
        .on_read_ready = readOnReady,
        .on_write_ready = writeOnReady,
    },
    {
        .state = UPDATE,
        .on_arrival = updateOnArrival,
        .on_write_ready = writeOnReady,
    },
    {
        .state = DONE,
    },
    {
        .state = ERROR
    }
};

//-----------------------------------Struct to storage relevant info for each client in selectorKey---------------------
clientData* newClientData(const struct sockaddr_storage clientAddress) {
    clientData* clientData = calloc(1, sizeof(struct clientData));

    clientData->stateMachine.initial = GREETINGS;
    clientData->stateMachine.max_state = ERROR;
    clientData->stateMachine.states = stateHandlers;
    stm_init(&clientData->stateMachine);

    uint8_t* readBuffer = malloc(BUFFER_SIZE);
    uint8_t* writeBuffer = malloc(BUFFER_SIZE);
    buffer_init(&clientData->readBuffer, BUFFER_SIZE, readBuffer);
    buffer_init(&clientData->writeBuffer, BUFFER_SIZE, writeBuffer);

    parserInit(&clientData->pop3Parser);

    clientData->currentUsername = NULL;
    clientData->isAuth = false;

    clientData->mailCount = 0;

    clientData->clientAddress = clientAddress;

    return clientData;
}

//-------------------------------Handlers for selector-----------------------------------------------
static void
pop3_done(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);

    selector_unregister_fd(key->s, key->fd);
    close(key->fd);

    if (mailDirectory != NULL)
        free(mailDirectory);
    free(data->readBuffer.data);
    free(data->writeBuffer.data);
    free(data);
}

static void
pop3_read(struct selector_key* key) {
    struct state_machine* stm = &ATTACHMENT(key)->stateMachine;
    const enum pop3_state st = stm_handler_read(stm, key);

    if (ERROR == st || DONE == st) {
        pop3_done(key);
    }
}

static void
pop3_write(struct selector_key* key) {
    struct state_machine* stm = &ATTACHMENT(key)->stateMachine;
    const enum pop3_state st = stm_handler_write(stm, key);

    if (ERROR == st || DONE == st) {
        pop3_done(key);
    }
}

static void
pop3_block(struct selector_key* key) {
    struct state_machine* stm = &ATTACHMENT(key)->stateMachine;
    const enum pop3_state st = stm_handler_block(stm, key);

    if (ERROR == st || DONE == st) {
        pop3_done(key);
    }
}

static const fd_handler pop3_handler = {
    .handle_read = pop3_read,
    .handle_write = pop3_write,
    .handle_block = pop3_block,
};

//------------------------------Passive Socket--------------------------------------------------------
void pop3_passive_accept(struct selector_key* key) {
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    const int client = accept(key->fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client == -1) {
        goto fail;
    }

    if (selector_fd_set_nio(client) == -1) {
        goto fail;
    }

    clientData* clientData = newClientData(client_addr);
    if (SELECTOR_SUCCESS != selector_register(key->s, client, &pop3_handler, OP_WRITE, clientData)) {
        goto fail;
    }
    return;

fail:
    if (client != -1) {
        close(client);
    }
}

void initMaildir(const char* directory) {
    mailDirectory = strdup(directory);
}

void writeInBuffer(struct selector_key* key, bool hasStatusCode, bool isError, char* msg, long len) {
    clientData* data = ATTACHMENT(key);

    size_t writable;
    uint8_t* writeBuffer;

    if (hasStatusCode == true) {
        char* status = isError ? ERROR_MSG : SUCCESS_MSG;

        writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
        memcpy(writeBuffer, status, strlen(status));
        buffer_write_adv(&data->writeBuffer, strlen(status));
    }

    if (msg != NULL && len > 0) {
        writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
        memcpy(writeBuffer, msg, len);
        buffer_write_adv(&data->writeBuffer, len);
    }

    writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
    writeBuffer[0] = '\r';
    writeBuffer[1] = '\n';
    buffer_write_adv(&data->writeBuffer, 2);
}

bool sendFromBuffer(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);

    size_t readable;
    uint8_t* readBuffer = buffer_read_ptr(&data->writeBuffer, &readable);

    ssize_t writeCount = send(key->fd, readBuffer, readable, 0);

    buffer_read_adv(&data->writeBuffer, writeCount);

    return readable == writeCount;
}

bool readAndParse(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);

    size_t readLimit;
    uint8_t* readBuffer = buffer_write_ptr(&data->readBuffer, &readLimit);
    const ssize_t readCount = recv(key->fd, readBuffer, readLimit, 0);
    buffer_write_adv(&data->readBuffer, readCount);

    while (!parserIsFinished(&data->pop3Parser) && buffer_can_read(&data->readBuffer))
        parse_feed(&data->pop3Parser, buffer_read(&data->readBuffer));

    if (parserIsFinished(&data->pop3Parser)) {
        printf("terminooooo");
    } else
        printf("no terminooooo");
    return parserIsFinished(&data->pop3Parser);
}
