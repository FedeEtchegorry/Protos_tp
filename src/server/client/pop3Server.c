#include "pop3Server.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/stat.h>

#include "pop3Parser.h"

#include "../greetings.h"
#include "../update.h"
#include "../parserUtils.h"
#include "../core/buffer.h"
#include "../core/stm.h"
#include "../logging/auth.h"
#include "../logging/metrics.h"
#include "../logging/logger.h"

char * mailDirectory = NULL;
extern server_configuration clientServerConfig;
extern server_metrics *clientMetrics;
extern server_logger *logger;
static char infoToLog[256];

//------------------------------------- Array de estados para la stm ---------------------------------------------------

static const struct state_definition stateHandlers[] = {
    {
        .state = GREETINGS,
        .on_arrival = greetingOnArrival,
        .on_write_ready = writeOnReadyPop3
    },
    {
        .state = AUTHORIZATION,
        .on_arrival = authOnArrival,
        .on_read_ready = readOnReadyPop3,
        .on_write_ready = writeOnReadyPop3,
    },
    {
        .state = TRANSACTION,
        .on_arrival = transactionOnArrival,
        .on_read_ready = readOnReadyPop3,
        .on_write_ready = writeOnReadyPop3,
    },
    {
        .state = TRANSFORMATION,
        .on_write_ready = writeOnReadyPop3,
    },
    {
        .state = UPDATE,
        .on_arrival = updateOnArrival,
        .on_write_ready=writeOnReadyPop3,
    },
    {
        .state = EXIT,
        .on_arrival = exitOnArrival,
        .on_write_ready=writeOnReadyPop3,
    },
    {
        .state = DONE,
    },
    {
        .state = ERROR,
        .on_arrival = errorOnArrival
    }
};

//----------------------------------- Struct to storage relevant info for each client in selectorKey -------------------

clientData* newClientData(const struct sockaddr_storage clientAddress) {
    clientData* clientData = calloc(1, sizeof(struct clientData));

    clientData->data.stateMachine.initial = GREETINGS;
    clientData->data.stateMachine.max_state = ERROR;
    clientData->data.stateMachine.states = stateHandlers;
    stm_init(&clientData->data.stateMachine);

    uint8_t* readBuffer = malloc(clientServerConfig.ioReadBufferSize);
    uint8_t* writeBuffer = malloc(clientServerConfig.ioWriteBufferSize);

    buffer_init(&clientData->data.readBuffer, clientServerConfig.ioWriteBufferSize, readBuffer);
    buffer_init(&clientData->data.writeBuffer, clientServerConfig.ioWriteBufferSize, writeBuffer);
    buffer_init(&clientData->data.readBuffer, clientServerConfig.ioReadBufferSize, readBuffer);
    buffer_init(&clientData->data.writeBuffer, clientServerConfig.ioWriteBufferSize, writeBuffer);

    const methodsMap *map = getPop3Methods();
    clientData->data.parser = parserInit(map);

    clientData->data.currentUsername = NULL;
    clientData->data.isAuth = false;
    clientData->data.closed = false;
    clientData->mailCount = 0;
    clientData->data.sockaddrStorage = clientAddress;

    return clientData;
}

static void freeClientData(struct clientData* clientData) {
    if (clientData->data.currentUsername != NULL)
        free(clientData->data.currentUsername);
    for (unsigned i =0; i<clientData->mailCount; i++) {
        free(clientData->mails[i]->filename);
        free(clientData->mails[i]);
    }
    parserDestroy(clientData->data.parser);
    free(clientData->data.readBuffer.data);
    free(clientData->data.writeBuffer.data);
    free(clientData);
}

//------------------------------ Passive Socket ------------------------------------------------------------------------

static void pop3_done(struct selector_key* key) {
    userData * data = ATTACHMENT_USER(key);
    if (data->closed)
        return;
    data->closed = true;

    selector_unregister_fd(key->s, key->fd);
    close(key->fd);

    snprintf(infoToLog, sizeof(infoToLog), "User '%s' has exit", data->currentUsername);
    serverLoggerRegister(logger, infoToLog);

    freeClientData(key->data);
}
void pop3_read(struct selector_key* key){
    struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
    const enum states_from_stm st = stm_handler_read(stm, key);

    if (ERROR == st || DONE == st) {
        pop3_done(key);
    }
}
void pop3_write(struct selector_key* key){
    struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
    const enum states_from_stm st = stm_handler_write(stm, key);

    if (ERROR == st || DONE == st) {
        pop3_done(key);
    }
}
void pop3_block(struct selector_key* key){
    struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
    const enum states_from_stm st = stm_handler_block(stm, key);

    if (ERROR == st || DONE == st) {
        pop3_done(key);
    }
}
void pop3_close(struct selector_key* key) {
    struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
    stm_handler_close(stm, key);
    pop3_done(key);
}


static fd_handler handler = {
    .handle_read = pop3_read,
    .handle_write = pop3_write,
    .handle_block = pop3_block,
    .handle_close = pop3_close,
};

void pop3PassiveAccept(struct selector_key* key) {

    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    clientData* clientData = NULL;

    const int client = accept(key->fd, (struct sockaddr*)&client_addr, &client_addr_len);

    if (client == -1) {
        goto fail;
    }

    if (selector_fd_set_nio(client) == -1) {
        goto fail;
    }

    clientData = newClientData(client_addr);

    if (SELECTOR_SUCCESS != selector_register(key->s, client, &handler, OP_WRITE, clientData)) {
        goto fail;
    }

    serverMetricsRecordNewConection(clientMetrics);

    return;

fail:
    if (client != -1) {
        close(client);
    }
    if (clientData != NULL) {
        freeClientData(clientData);
    }
}

void initPOP3Config(struct pop3Config config) {
    mkdir(config.maildir, 0755);
    mailDirectory = strdup(config.maildir);

    for (unsigned int i = 0; i < config.nusers; i++) {
        addUser(config.users[i].name, config.users[i].pass, config.users[i].role);
    }
}

//------------------------------------- Generic handler (just to support pipelining)-------------------------

unsigned readOnReadyPop3(struct selector_key * key) {
    userData * data = ATTACHMENT_USER(key);
    bool isFinished = readAndParse(key);
    if (isFinished) {
        unsigned next = UNKNOWN;
        switch (stm_state(&data->stateMachine)) {
        case AUTHORIZATION:
          next = authOnReadReady(key);
          break;
        case TRANSACTION:
          next= transactionOnReadReady(key);
          break;
        }
        resetParser(data->parser);
        selector_set_interest_key(key, OP_WRITE);
        return next;
    }
    return stm_state(&data->stateMachine);
}

unsigned writeOnReadyPop3(struct selector_key * key) {
    userData * data = ATTACHMENT_USER(key);
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
        case TRANSFORMATION:
            if (!data->isEmailFinished) {
                //wait to transformator process to tell me when there is something to read in buffer
                selector_set_interest_key(key, OP_NOOP);
                return TRANSFORMATION;
            }
            next = TRANSACTION;
            break;
        case UPDATE:
        case EXIT:
          next = DONE;
          break;
        default:
            return ERROR;
        }

        if (!buffer_can_read(&data->readBuffer)) {
          selector_set_interest_key(key, OP_READ);
          return next;
        }
        jump(&data->stateMachine, next, key);
        selector_set_interest_key(key, OP_READ);
        readOnReadyPop3(key);
    }
    return stm_state(&data->stateMachine);
}