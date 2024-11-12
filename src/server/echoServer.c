#include "echoServer.h"
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "selector.h"

#define READ_BUFFER_SIZE 8192
#define MAX_HOSTNAME_LENGTH 255

#define ATTACHMENT(key) ((struct buffer *)key->data)

static void echo_read(struct selector_key* key);
static void echo_write(struct selector_key* key);
static void echo_block(struct selector_key* key);
static void echo_close(struct selector_key* key);
static const struct fd_handler echo_handler = {
    .handle_read = echo_read,
    .handle_write = echo_write,
    .handle_close = echo_block,
    .handle_block = echo_close,
};

void passive_accept(struct selector_key* key)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct socks5* state = NULL;

    const int client = accept(key->fd, (struct sockaddr*)&client_addr,
                              &client_addr_len);
    if (client == -1)
    {
        goto fail;
    }

    if (selector_fd_set_nio(client) == -1)
    {
        goto fail;
    }

    buffer * buffer = malloc(sizeof(struct buffer));
    uint8_t * data = malloc(READ_BUFFER_SIZE);
    buffer_init(buffer, READ_BUFFER_SIZE, data);


    if (SELECTOR_SUCCESS != selector_register(key->s, client, &echo_handler,
                                              OP_READ, buffer))
    {
        goto fail;
    }
    return;

fail:
    if (client != -1)
    {
        close(client);
    }
}

static void echo_read(struct selector_key* key)
{

}

static void echo_write(struct selector_key* key)
{

}

static void echo_block(struct selector_key* key)
{

}

static void echo_close(struct selector_key* key)
{

}

int handleEchoClient(const int clientFd)
{
    char buffer[READ_BUFFER_SIZE];
    ssize_t bytesRead = recv(clientFd, buffer, READ_BUFFER_SIZE, 0);
    if (bytesRead < 0)
    {
        perror("recv");
        return -1;
    }

    while (bytesRead > 0)
    {
        const ssize_t bytesWritten = send(clientFd, buffer, bytesRead, 0);
        if (bytesWritten < 0)
        {
            perror("send");
            return -1;
        }
        if (bytesWritten != bytesRead)
        {
            perror("send");
            return -1;
        }
        bytesRead = recv(clientFd, buffer, READ_BUFFER_SIZE, 0);
        if (bytesRead < 0)
        {
            perror("recv");
            return -1;
        }
    }

    close(clientFd);
    return 0;
}
