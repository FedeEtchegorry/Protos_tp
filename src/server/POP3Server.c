#include "POP3Server.h"

#include <stdint.h>
#include <stdlib.h>

#include "selector.h"

#include <sys/socket.h>

#include "buffer.h"
#include "stm.h"
#include "parser.h"

#define BUFFER_SIZE 8192
#define MAX_HOSTNAME_LENGTH 255

//---------------------------------------------POP3 States-----------------------------------------------------
enum pop3_state {
    /* Reads and processes the client authentication.
    Interests:
        - OP_READ -> client_fd
    Transitions:
        - AUTH_READ if the message was not completely read
        - AUTH_WRITE when the message is completely read
        - ERROR if an error occurs (IO/parsing) */
    GREETINGS,
    /* Reads and processes the client authentication.
    Interests:
        - OP_READ -> client_fd
    Transitions:
        - AUTH_READ if the message was not completely read
        - AUTH_WRITE when the message is completely read
        - ERROR if an error occurs (IO/parsing) */
    USER_READ,

    /* Reads and processes the client authentication.
    Interests:
        - OP_READ -> client_fd
    Transitions:
        - AUTH_READ if the message was not completely read
        - AUTH_WRITE when the message is completely read
        - ERROR if an error occurs (IO/parsing) */
    USER_WRITE,

    /* Reads and processes the client authentication.
    Interests:
        - OP_READ -> client_fd
    Transitions:
        - PASS_READ if the message was not completely read
        - AUTH_WRITE when the message is completely read
        - ERROR if an error occurs (IO/parsing) */
    PASS_READ,

    /* Sends the authentication answer to the client
    Interests:
       - OP_WRITE -> client_fd
    Transitions:
       - AUTH_WRITE if there are bytes to be sended
       - REQUEST_READ when all the bytes where sended, and the auth provided was valid
       - ERROR if an error occurs (IO/parsing) */
    PASS_WRITE,

    /* Reads and processes the client request.
    Interests:
        - OP_READ -> client_fd
    Transitions:
        - METHOD_READ if the message was not completely read
        - METHOD_WRITE if there were errors processing the request
        - ERROR if an error occurs (IO/parsing) */
    METHOD_READ,

    /* Sends the request answer to the client
    Interests:
        - OP_WRITE -> client_fd
    Transitions:
        - METHOD_WRITE if there are bytes to be sended
        - METHOD_READ if the request was successful
        - ERROR I/O error */
    METHOD_WRITE,

    // Terminal states
    DONE,
    ERROR,
};

//-------------------------------------Array de estados para la stm------------------------------------------
static const struct state_definition stateHandlers[] = {
    {
        .state = GREETINGS,
    },
    {
        .state = USER_READ,
    },
    {
        .state = USER_WRITE
    },
    {
        .state = PASS_READ
    },
    {
        .state = PASS_WRITE
    },
    {
        .state = METHOD_READ
    },
    {
        .state = METHOD_WRITE
    },
    {
        .state = DONE
    },
    {
        .state = ERROR
    }
};

//-----------------------------------Struct to storage relevant info for each client in selectorKey---------------------

clientData* newClientData(const struct sockaddr_storage clientAddress) {
    clientData* clientData = calloc(1, sizeof(struct clientData));

    clientData->stateMachine.initial = USER_READ;
    clientData->stateMachine.max_state = ERROR;
    clientData->stateMachine.states = stateHandlers;
    stm_init(&clientData->stateMachine);

    uint8_t * readBuffer = malloc(BUFFER_SIZE);
    uint8_t * writeBuffer = malloc(BUFFER_SIZE);
    buffer_init(&clientData->readBuffer, BUFFER_SIZE, readBuffer);
    buffer_init(&clientData->writeBuffer, BUFFER_SIZE, writeBuffer);

    clientData->isAuth = false;
    clientData->clientAddress = clientAddress;

    return clientData;
}

//-------------------------------Handlers for selector-----------------------------------------------
static void
pop3_done(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);

    selector_unregister_fd(key->s, key->fd);
    close(key->fd);

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

static void
pop3_close(struct selector_key* key) {
    struct state_machine* stm = &ATTACHMENT(key)->stateMachine;
    stm_handler_close(stm, key);
    pop3_done(key);
}

static const fd_handler pop3_handler = {
    .handle_read = pop3_read,
    .handle_write = pop3_write,
    .handle_close = pop3_close,
    .handle_block = pop3_block,
};

//------------------------------Passive Socket--------------------------------------------------------
void pop3_passive_accept(const struct selector_key* key) {
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
    if (SELECTOR_SUCCESS != selector_register(key->s, client, &pop3_handler, OP_READ, clientData)) {
        goto fail;
    }
    return;

fail:
    if (client != -1) {
        close(client);
    }
}

