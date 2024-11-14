#include "POP3Server.h"

#include <stdint.h>
#include <stdlib.h>

#include "utils/selector.h"

#include <bits/socket.h>
#include <sys/socket.h>

#include "utils/buffer.h"
#include "utils/stm.h"
#include "utils/parser.h"

#define READ_BUFFER_SIZE 8192
#define MAX_HOSTNAME_LENGTH 255
#define ATTACHMENT(key) ((clientData *)(key->data))

//-----------------------------------Struct to storage relevant info for each client in selectorKey---------------------
typedef struct clientData {
    buffer* readBuffer;
    buffer* writeBuffer;
    struct state_machine* stateMachine;
} clientData;

clientData* newClientData() {
    //TODO
}

//---------------------------------------------POP3 States-----------------------------------------------------
enum pop3_state {
    /**
     * recibe el mensaje `hello` del cliente, y lo procesa
     *
     * Intereses:
     *     - OP_READ sobre client_fd
     *
     * Transiciones:
     *   - HELLO_READ  mientras el mensaje no esté completo
     *   - HELLO_WRITE cuando está completo
     *   - ERROR       ante cualquier error (IO/parseo)
     */
    HELLO_READ,

    /**
     * envía la respuesta del `hello' al cliente.
     *
     * Intereses:
     *     - OP_WRITE sobre client_fd
     *
     * Transiciones:
     *   - HELLO_WRITE  mientras queden bytes por enviar
     *   - REQUEST_READ cuando se enviaron todos los bytes
     *   - ERROR        ante cualquier error (IO/parseo)
     */
    HELLO_WRITE,

    …

    // estados terminales
    DONE,
    ERROR,
};

//-------------------------------------Array de estados para la stm------------------------------------------
static const struct state_definition client_statbl[] = {
    {
        .state            = HELLO_READ,
        .on_arrival       = hello_read_init,
        .on_departure     = hello_read_close,
        .on_read_ready    = hello_read,
    },
    {
        .state = HELLO_WRITE
    },
    {
        .state = DONE
    },
    {
        .state = ERROR
    }
};

//-------------------------------Handlers for selector-----------------------------------------------
static void pop3_read(struct selector_key* key);
static void pop3_write(struct selector_key* key);
static void pop3_block(struct selector_key* key);
static void pop3_close(const struct selector_key* key);
static const fd_handler pop3_handler = {
    .handle_read = pop3_read,
    .handle_write = pop3_write,
    .handle_close = pop3_close,
    .handle_block = pop3_block,
};


//------------------------------Passive Socket--------------------------------------------------------
void passive_accept(const struct selector_key* key) {
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    const int client = accept(key->fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client == -1) {
        goto fail;
    }

    if (selector_fd_set_nio(client) == -1) {
        goto fail;
    }

    clientData* clientData = newClientData();
    if (SELECTOR_SUCCESS != selector_register(key->s, client, &pop3_handler, OP_READ, clientData)) {
        goto fail;
    }
    return;

fail:
    if (client != -1) {
        close(client);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Handlers top level de la conexión pasiva.
// son los que emiten los eventos a la maquina de estados.
static void
pop3_done(struct selector_key* key) {
    //TODO abortar la ejecucion o algo parecido
    const int fds[] = {
        ATTACHMENT(key)->client_fd,
        ATTACHMENT(key)->origin_fd,
    };
    for (unsigned i = 0; i < N(fds); i++) {
        if (fds[i] != -1) {
            if (SELECTOR_SUCCESS != selector_unregister_fd(key->s, fds[i])) {
                abort();
            }
            close(fds[i]);
        }
    }
}

static void
pop3_read(struct selector_key* key) {
    struct state_machine* stm = ATTACHMENT(key)->stateMachine;
    const enum socks_v5state st = stm_handler_read(stm, key);

    if (ERROR == st || DONE == st) {
        pop3_done(key);
    }
}

static void
pop3_write(struct selector_key* key) {
    struct state_machine* stm = ATTACHMENT(key)->stateMachine;
    const enum socks_v5state st = stm_handler_write(stm, key);

    if (ERROR == st || DONE == st) {
        pop3_done(key);
    }
}

static void
pop3_block(struct selector_key* key) {
    struct state_machine* stm = ATTACHMENT(key)->stateMachine;
    const enum socks_v5state st = stm_handler_block(stm, key);

    if (ERROR == st || DONE == st) {
        pop3_done(key);
    }
}

static void
pop3_close(struct selector_key* key) {
    socks5_destroy(ATTACHMENT(key));
}
