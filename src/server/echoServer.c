#include "echoServer.h"
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>


#include "buffer.h"
#include "selector.h"

#define READ_BUFFER_SIZE 8192
#define MAX_HOSTNAME_LENGTH 255
#define ATTACHMENT(key) ((buffer *)(key->data))


//-------------------------------Handlers for selector-----------------------------------------------
static void echo_read(struct selector_key* key);
static void echo_write(struct selector_key* key);
static void echo_block(struct selector_key* key);
static void echo_close(const struct selector_key* key);
static const fd_handler echo_handler = {
    .handle_read = echo_read,
    .handle_write = echo_write,
    .handle_close = echo_close,
    .handle_block = echo_block,
};


//------------------------------Passive Socket--------------------------------------------------------
void passive_accept(const struct selector_key* key) {
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    const int client = accept(key->fd, (struct sockaddr*)&client_addr,
                              &client_addr_len);
    if (client == -1) {
        goto fail;
    }

    if (selector_fd_set_nio(client) == -1) {
        goto fail;
    }

    buffer* buffer = malloc(sizeof(struct buffer));
    uint8_t* data = malloc(READ_BUFFER_SIZE);
    buffer_init(buffer, READ_BUFFER_SIZE, data);

    if (SELECTOR_SUCCESS != selector_register(key->s, client, &echo_handler, OP_READ, buffer)) {
        goto fail;
    }
    return;

fail:
    if (client != -1) {
        close(client);
    }
}

//----------------------------------Implementation of handlers-------------------------------------------
static void echo_read(struct selector_key* key) {
    buffer* bufferData = ATTACHMENT(key);
    size_t size;
    uint8_t* writePointer = buffer_write_ptr(bufferData, &size);

    const ssize_t written = recv(key->fd, writePointer, size, 0);
    buffer_write_adv(bufferData, written);

    fd_interest interest = OP_NOOP;
    interest = interest | OP_WRITE;
    if (buffer_can_write(bufferData))
        interest = interest | OP_READ;

    selector_set_interest_key(key, interest);
}

static void echo_write(struct selector_key* key) {
    buffer* bufferData = ATTACHMENT(key);
    size_t size;
    const uint8_t* readPointer = buffer_read_ptr(bufferData, &size);

    const ssize_t written = send(key->fd, readPointer, size, 0);
    buffer_read_adv(bufferData, written);

    fd_interest interest = OP_NOOP;
    interest = interest | OP_READ;
    if (buffer_can_read(bufferData))
        interest = interest | OP_WRITE;

    selector_set_interest_key(key, interest);
}

static void echo_block(struct selector_key* key) {
    //unnecessary
}

static void echo_close(const struct selector_key* key) {
    buffer* bufferData = ATTACHMENT(key);
    free(bufferData->data);
    free(bufferData);
    selector_unregister_fd(key->s, key->fd);
    close(key->fd);
}
