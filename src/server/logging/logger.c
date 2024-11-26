#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define LOGGER_BUFFER_SIZE        8096
#define FILE_PERMISSIONS          666
#define ATTACHMENT(key)          ((server_logger *)(key->data))

// ---------------------------------------------- PRIVATE --------------------------------------------------------------

static void serverLoggerWrite(server_logger *serverLogger) {

    size_t written = 0;
    size_t toBeWritten;
    uint8_t *buffer = buffer_read_ptr(&serverLogger->buffer, &toBeWritten);

    written = write(serverLogger->loggerFd, buffer, toBeWritten);
    buffer_read_adv(&serverLogger->buffer, written);

    selector_set_interest(*(serverLogger->selector), serverLogger->loggerFd, (toBeWritten - written) > 0 ? OP_WRITE : OP_NOOP);

    if (written == toBeWritten) {
        buffer_compact(&serverLogger->buffer);
        DEBUG_PRINT_LOCATION();
    }

    DEBUG_PRINT_LOCATION();
}

static void serverLoggerWriteHandler(struct selector_key *key) {

    DEBUG_PRINT_LOCATION();
    server_logger *serverLogger = ATTACHMENT(key);
    serverLoggerWrite(serverLogger);
}

static void serverLoggerCloseHandler(struct selector_key *key) {

    server_logger *serverLogger = ATTACHMENT(key);

    int flags = fcntl(serverLogger->loggerFd, F_GETFD, 0);
    // Ahora configuramos bloqueante para asegurarnos escritura de lo que puede faltar al finalizar esta función
    fcntl(serverLogger->loggerFd, F_SETFL, flags & (~O_NONBLOCK));
    serverLoggerWriteHandler(key);
    int oldLoggerFd = serverLogger->loggerFd;
    serverLogger->loggerFd = -1;

    close(oldLoggerFd);
}

const struct fd_handler loggerFdHandler = {
    .handle_read = NULL,
    .handle_write = serverLoggerWriteHandler,
    .handle_block = NULL,
    .handle_close = serverLoggerCloseHandler,
};

// ---------------------------------------------- PUBLIC ---------------------------------------------------------------

server_logger *serverLoggerCreate(fd_selector *selector, char *loggerFilePath) {

    if (selector == NULL) {
        return NULL;
    }
    server_logger *serverLogger = malloc(sizeof(server_logger));
    memset(serverLogger, 0, sizeof(server_logger));

    serverLogger->selector = selector;
    serverLogger->bufferSize = LOGGER_BUFFER_SIZE;

    uint8_t *bufferData = malloc(serverLogger->bufferSize);
    buffer_init(&serverLogger->buffer, serverLogger->bufferSize, bufferData);

    serverLogger->loggerFd = loggerFilePath == NULL ? -1 : open(loggerFilePath, O_WRONLY | O_APPEND | O_CREAT | O_NONBLOCK, FILE_PERMISSIONS);

    if (serverLogger->loggerFd > 0) {
        serverLogger->loggerFilePath = loggerFilePath;
    }

    selector_register(*selector, serverLogger->loggerFd, &loggerFdHandler, OP_NOOP, serverLogger);

    return serverLogger;
}

void serverLoggerTerminate(server_logger **serverLogger) {

    if (serverLogger == NULL || *serverLogger == NULL) {
        return;
    }

    server_logger *oldServerLogger = *serverLogger;
    *serverLogger = NULL;

    selector_unregister_fd(*(oldServerLogger->selector), oldServerLogger->loggerFd);

    free(oldServerLogger->buffer.data);
    free(oldServerLogger);
}

void serverLoggerRegister(server_logger *serverLogger) {

    if (serverLogger == NULL) {
        return;
    }

    size_t maxBytes = serverLogger->buffer.limit - serverLogger->buffer.write;
    size_t toBeWritten = snprintf((char*)serverLogger->buffer.write, maxBytes, "123456789");
    buffer_write_adv(&serverLogger->buffer, toBeWritten);
    serverLoggerWrite(serverLogger);
}

size_t serverLoggerRetrive(server_logger *serverLogger, char *string, size_t lines) {

    // Que conviene hacer? No podria ser no bloqueante creo
}