#include "pop3Server.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "pop3Parser.h"

#include "../greetings.h"
#include "../update.h"
#include "../parserUtils.h"
#include "../core/buffer.h"
#include "../core/stm.h"
#include "../logging/auth.h"

char * mailDirectory = NULL;
extern server_configuration clientServerConfig;

//------------------------------------- Array de estados para la stm ---------------------------------------------------

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

//----------------------------------- Struct to storage relevant info for each client in selectorKey -------------------

clientData* newClientData(const struct sockaddr_storage clientAddress) {

    clientData* clientData = calloc(1, sizeof(struct clientData));

    clientData->data.stateMachine.initial = GREETINGS;
    clientData->data.stateMachine.max_state = ERROR;
    clientData->data.stateMachine.states = stateHandlers;
    stm_init(&clientData->data.stateMachine);

    uint8_t* readBuffer = malloc(clientServerConfig.ioReadBufferSize);
    uint8_t* writeBuffer = malloc(clientServerConfig.ioWriteBufferSize);

    buffer_init(&clientData->data.readBuffer, clientServerConfig.ioReadBufferSize, readBuffer);
    buffer_init(&clientData->data.writeBuffer, clientServerConfig.ioWriteBufferSize, writeBuffer);

    const methodsMap *map = getPop3Methods();

    parserInit(&clientData->data.parser, map);

    clientData->data.currentUsername = NULL;
    clientData->data.isAuth = false;
    clientData->data.closed = false;
    clientData->mailCount = 0;
    clientData->data.sockaddrStorage = clientAddress;

    return clientData;
}

void freeClientData(struct clientData** clientData) {

    if (clientData == NULL || *clientData == NULL) {
        return;
    }
    struct clientData *oldClientData = *clientData;
    *clientData = NULL;

    if ((*clientData)->data.readBuffer.data != NULL) {
        free(oldClientData->data.readBuffer.data);
        oldClientData->data.readBuffer.data = NULL;
    }
    if ((*clientData)->data.writeBuffer.data != NULL) {
        free(oldClientData->data.writeBuffer.data);
        oldClientData->data.writeBuffer.data = NULL;
    }
}

//------------------------------ Passive Socket ------------------------------------------------------------------------

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

    if (SELECTOR_SUCCESS != selector_register(key->s, client, getHandler(), OP_WRITE, clientData)) {
        goto fail;
    }

    return;

fail:
    if (client != -1) {
        close(client);
        // Registrar con matrics
    }
    if (clientData != NULL) {
        freeClientData(&clientData);
    }
}

void initMaildir(const char* directory) {
    mailDirectory = strdup(directory);
}
