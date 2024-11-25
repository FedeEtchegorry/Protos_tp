#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define LOGGER_BUFFER_SIZE        8096
#define FILE_PERMISSIONS          666

const struct fd_handler loggerFdHandler = {
    .handle_read = NULL,
    .handle_write = NULL,
    .handle_block = NULL,
    .handle_close = NULL,
};

// ---------------------------------------------- PRIVATE --------------------------------------------------------------

static void serverLoggerWriteHandler() {

}

static void serverLoggerCloseHandler() {

}

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

    selector_register(*selector, serverLogger->loggerFd, &loggerFdHandler, OP_NOOP, NULL);

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

}

size_t serverLoggerRetrive(server_logger *serverLogger, char *string, size_t lines) {



}