#include "metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../serverConfigs.h"

#define EMPTY_METRIC_LINE_SIZE    7

server_metrics *serverMetricsCreate(char *dataFilePath, const size_t *ioReadBufferSize, const size_t *ioWriteBufferSize) {

    server_metrics *serverMetrics = malloc(sizeof(server_metrics));

    if (serverMetrics == NULL) {
      return NULL;
    }

    memset(serverMetrics, 0, sizeof(server_metrics));
    serverMetrics->ioReadBufferSize = ioReadBufferSize;
    serverMetrics->ioWriteBufferSize = ioWriteBufferSize;

    if (dataFilePath == NULL) {
        return serverMetrics;
    }

    FILE *file = NULL;

    int fileAlreadyExists = access(dataFilePath, F_OK) == 0;

    if (fileAlreadyExists) {

        char line[64] = {0};
        size_t fileSize = 0;
        long position = 0;

        file = fopen(dataFilePath, "r+");
        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        position = fileSize - 1;

        while (position >= 0) {

            fseek(file, position, SEEK_SET);

            if (position == 0 || (fgetc(file) == '\n' && position != fileSize - 1)) {
                fgets(line, sizeof(line), file);
                break;
            }
            position--;
        }

        if (line[0] && line[0] != '\n' && line[0] != '\r') {

            serverMetrics->totalCountConnections = strtol(strtok(line, ";"), NULL, 10);
            serverMetrics->totalTransferredBytes = strtol(strtok(NULL, ";"), NULL, 10);
            serverMetrics->totalReceivedBytes = strtol(strtok(NULL, ";"), NULL, 10);
            serverMetrics->totalCountUsers = strtol(strtok(NULL, "\n"), NULL, 10);
        }
    }
    else {
        fprintf(stderr,"Creando archivo %s\n", dataFilePath);
        file = fopen(dataFilePath, "w");
        serverMetricsRecordInFile(serverMetrics);
    }

    if (file == NULL) {
        fprintf(stderr,"Error al abrir el archivo %s\n", dataFilePath);
    }
    else {
        fclose(file);
        serverMetrics->dataFilePath = dataFilePath;

        if (!fileAlreadyExists) {
            serverMetricsRecordInFile(serverMetrics);
        }
    }

    return serverMetrics;
}

void serverMetricsFree(server_metrics** metrics) {

    if (metrics == NULL || *metrics == NULL) {
      return;
    }
    free(*metrics);
}

void serverMetricsRecordNewConection(server_metrics* metrics) {

    if (metrics == NULL) {
      return;
    }

    metrics->totalCountConnections++;
    metrics->currentConectionsCount++;
}

void serverMetricsRecordDropConection(server_metrics* metrics) {

    if (metrics == NULL || metrics->currentConectionsCount == 0) {
      return;
    }

    metrics->currentConectionsCount--;
}

void serverMetricsRecordBytesTransferred(server_metrics* metrics, size_t bytes) {

    if (metrics == NULL || (metrics->totalTransferredBytes + bytes) <= metrics->totalTransferredBytes) {
      return;
    }

    metrics->totalTransferredBytes += bytes;
}

void serverMetricsRecordBytesReceived(server_metrics* metrics, size_t bytes) {

    if (metrics == NULL || (metrics->totalReceivedBytes + bytes) <= metrics->totalReceivedBytes) {
        return;
    }

    metrics->totalReceivedBytes += bytes;
}

void serverMetricsReset(server_metrics* metrics) {

    if (metrics == NULL) {
        return;
    }
    // Salvar puntero de lista de usuarios
    memset(metrics, 0, sizeof(server_metrics));
    serverMetricsRecordNewConection(metrics);
}

int serverMetricsRecordInFile(server_metrics *metrics) {

    if (metrics == NULL || metrics->dataFilePath == NULL) {
        return 0;
    }

    FILE *file = fopen(metrics->dataFilePath, "a+");

    if (file == NULL) {
        fprintf(stderr,"Error al abrir el archivo %s\n", metrics->dataFilePath);
        return 0;
    }

    fprintf(file, "%lu;%lu;%lu;%lu\n", metrics->totalCountConnections, metrics->totalTransferredBytes, metrics->totalReceivedBytes, metrics->totalCountUsers);
    fflush(file);
    fclose(file);

    return 1;
}
