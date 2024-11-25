#include "metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../serverConfigs.h"

server_metrics *serverMetricsCreate(char *dataFilePath, const size_t *ioReadBufferSize, const size_t *ioWriteBufferSize) {

    server_metrics *serverMetrics = malloc(sizeof(server_metrics));

    if (serverMetrics == NULL) {
      return NULL;
    }

    memset(serverMetrics, 0, sizeof(server_metrics));
    serverMetrics->ioReadBufferSize = ioReadBufferSize;
    serverMetrics->ioWriteBufferSize = ioWriteBufferSize;

    if (dataFilePath != NULL) {

        FILE *file = NULL;

        if (access(dataFilePath, F_OK) != 0) {
            file = fopen(serverMetrics->dataFilePath, "w");
        }
        else {
            file = fopen(serverMetrics->dataFilePath, "w+");
        }

        if (file != NULL) {

            char line[64] = {0};

            while (fgets(line, sizeof(line), file)) {

                if (line[0] != '\n' && line[0] != '\r') {

                    serverMetrics->totalCountConnections = strtol(strtok(line, ";"), NULL, 12);
                    serverMetrics->totalTransferredBytes = strtol(strtok(line, ";"), NULL, 12);
                    serverMetrics->totalReceivedBytes = strtol(strtok(line, ";"), NULL, 12);
                    serverMetrics->totalCountUsers = strtol(strtok(line, "\n"), NULL, 12);
                }
            }

            fclose(file);
            serverMetrics->dataFilePath = dataFilePath;
        }
        else {
            fprintf(stderr,"Error al abrir el archivo %s\n", serverMetrics->dataFilePath);
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

int serverMetricsRecordInFile(server_metrics *metrics) {

    if (metrics != NULL || metrics->dataFilePath == NULL) {
        return 0;
    }

    FILE *file = fopen(metrics->dataFilePath, "w+");

    if (file == NULL) {
        fprintf(stderr,"Error al abrir el archivo %s\n", metrics->dataFilePath);
        return 0;
    }

    fprintf(file, "%lu;%lu;%lu;%lu\n", metrics->totalCountConnections, metrics->totalTransferredBytes, metrics->totalReceivedBytes, metrics->totalCountUsers);
    fclose(file);

    return 1;
}
