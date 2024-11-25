#ifndef LOGGER_H
#define LOGGER_H

#include "../serverConfigs.h"
#include "../core/selector.h"

typedef struct server_logger {

    char *loggerFilePath;

} server_logger;

int serverLoggerInit();

int serverLoggerTerminate();

void serverLoggerRegister();

#endif // LOGGER_H