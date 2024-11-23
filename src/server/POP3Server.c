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

    clientData->data.stateMachine.initial = GREETINGS;
    clientData->data.stateMachine.max_state = ERROR;
    clientData->data.stateMachine.states = stateHandlers;
    stm_init(&clientData->data.stateMachine);

    uint8_t* readBuffer = malloc(BUFFER_SIZE);
    uint8_t* writeBuffer = malloc(BUFFER_SIZE);
    buffer_init(&clientData->data.readBuffer, BUFFER_SIZE, readBuffer);
    buffer_init(&clientData->data.writeBuffer, BUFFER_SIZE, writeBuffer);

    parserInit(&clientData->data.pop3Parser);

    clientData->data.currentUsername = NULL;
    clientData->data.isAuth = false;
    clientData->data.closed = false;

    clientData->mailCount = 0;

    clientData->data.sockaddrStorage = clientAddress;

    return clientData;
}

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
    if (SELECTOR_SUCCESS != selector_register(key->s, client, getHandler(), OP_WRITE, clientData)) {
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
