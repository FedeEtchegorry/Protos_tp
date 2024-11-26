#ifndef LOGGER_H
#define LOGGER_H

#include "../serverConfigs.h"
#include "../core/selector.h"
#include "../core/buffer.h"

typedef struct server_logger {

    fd_selector *selector;
    buffer buffer;
    size_t bufferSize;
    size_t currentLogs;
    char *loggerFilePath;
    int loggerFd;

} server_logger;

server_logger *serverLoggerCreate(fd_selector *selector, char *loggerFilePath);

void serverLoggerTerminate(server_logger **serverLogger);

void serverLoggerRegister(server_logger *serverLogger, char *stringData);

unsigned long serverLoggerRetrieve(server_logger *serverLogger, char *string, unsigned long maxBytes, unsigned long lines);

#endif // LOGGER_H