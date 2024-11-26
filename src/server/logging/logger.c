#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define LOGGER_BUFFER_SIZE        8096
#define FILE_PERMISSIONS 		 (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
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
}

static void serverLoggerClose(server_logger *serverLogger) {

    int flags = fcntl(serverLogger->loggerFd, F_GETFD, 0);
    // Ahora configuramos bloqueante para asegurarnos escritura de lo que puede faltar al finalizar esta función
    fcntl(serverLogger->loggerFd, F_SETFL, flags & (~O_NONBLOCK));

    if (buffer_can_read(&serverLogger->buffer)) {
        serverLoggerWrite(serverLogger);
    }

    int oldLoggerFd = serverLogger->loggerFd;
    serverLogger->loggerFd = -1;

    fsync(oldLoggerFd);
    close(oldLoggerFd);
}

static void serverLoggerWriteHandler(struct selector_key *key) {

    server_logger *serverLogger = ATTACHMENT(key);
    serverLoggerWrite(serverLogger);
}

static void serverLoggerCloseHandler(struct selector_key *key) {

    server_logger *serverLogger = ATTACHMENT(key);
    serverLoggerClose(serverLogger);
}

const struct fd_handler loggerFdHandler = {
    .handle_read = NULL,
    .handle_write = serverLoggerWriteHandler,
    .handle_block = NULL,
    .handle_close = serverLoggerCloseHandler
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

    serverLogger->loggerFd = loggerFilePath == NULL ? -1 : open(loggerFilePath, O_WRONLY | O_APPEND | O_CREAT, FILE_PERMISSIONS);

    if (serverLogger->loggerFd > 0) {
        serverLogger->loggerFilePath = loggerFilePath;
        selector_register(*selector, serverLogger->loggerFd, &loggerFdHandler, OP_NOOP, serverLogger); // Cerrar en caso de fallo
    }

    return serverLogger;
}

void serverLoggerTerminate(server_logger **serverLogger) {

    if (serverLogger == NULL || *serverLogger == NULL) {
        return;
    }

    server_logger *oldServerLogger = *serverLogger;
    *serverLogger = NULL;

    if (buffer_can_read(&oldServerLogger->buffer)) {
        serverLoggerClose(oldServerLogger);
    }
    selector_unregister_fd(*(oldServerLogger->selector), oldServerLogger->loggerFd);

    free(oldServerLogger->buffer.data);
    free(oldServerLogger);
}

void serverLoggerRegister(server_logger *serverLogger, char *stringData) {

    if (serverLogger == NULL) {
        return;
    }
    size_t maxBytes = serverLogger->buffer.limit - serverLogger->buffer.write;
    size_t toBeWritten = snprintf((char*)serverLogger->buffer.write, maxBytes, "[OUTPUT] %s\n", stringData);
    buffer_write_adv(&serverLogger->buffer, toBeWritten);
    serverLoggerWrite(serverLogger);
}

unsigned long serverLoggerRetrieve(server_logger *serverLogger, char *stringData, unsigned long maxBytes, unsigned long lines) {
    // Este es bloqueante, se usa solo bajo demanda de comando; mantener al minimo
    if (serverLogger == NULL || stringData == NULL || lines == 0) {
        return 0;
    }
    int fd = open(serverLogger->loggerFilePath, O_RDONLY);

    if (fd < 0) {
        return 0;
    }

    char buffer[1024];
    ssize_t bytesRead = 0;
    char *lineStart = buffer;
    long index = 0;
    long totalLines = 0;
    long totalBytes = 0;
    long offset = lseek(fd, 0, SEEK_END);

    while (offset > 0 && totalLines < lines) {
		// Leer de atrás para adelante el archivo
        size_t chunkSize = sizeof(buffer);

        if (offset < chunkSize) {
            chunkSize = offset;
        }
        offset = lseek(fd, -chunkSize, SEEK_CUR);
		bytesRead = read(fd, buffer, chunkSize);

        if (offset < 0 || bytesRead <= 0) {
            break;
        }

        for (index = bytesRead - 1; index >= 0 && totalLines < lines; index--) {
          	// Procesar el bloque de atrás hacia adelante
            if (buffer[index] == '\n' && index != bytesRead - 1) {
                totalLines++;
            }
            if (totalLines <= lines && totalBytes < maxBytes - 1 && index != bytesRead - 1) {
                stringData[totalBytes] = buffer[index];
                totalBytes++;
            }
        }
    }

    close(fd);
    // Invertir la información que está inversa xd
    for (index = 0; index < totalBytes / 2; index++) {
        char temp = stringData[index];
        stringData[index] = stringData[totalBytes - index - 1];
        stringData[totalBytes - index - 1] = temp;
    }

	stringData[totalBytes] = '\0';

    return totalLines;
}