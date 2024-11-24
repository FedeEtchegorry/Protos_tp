#include "metrics.h"
#include <stdlib.h>
#include <string.h>

server_metrics *serverMetricsCreate(server_metrics* metrics, size_t *ioReadBufferSize, size_t *ioWriteBufferSize) {

    server_metrics *serverMetrics = malloc(sizeof(server_metrics));

    if (serverMetrics == NULL) {
      return NULL;
    }

    memset(serverMetrics, 0, sizeof(server_metrics));
    serverMetrics->ioReadBufferSize = ioReadBufferSize;
    serverMetrics->ioWriteBufferSize = ioWriteBufferSize;

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

    metrics->historicConectionsCount++;
    metrics->currentConectionsCount++;
}

void serverMetricsRecordDropConection(server_metrics* metrics) {

    if (metrics == NULL || metrics->currentConectionsCount == 0) {
      return;
    }

    metrics->currentConectionsCount--;
}

void serverMetricsRecordBytesTransferred(server_metrics* metrics, size_t bytes) {

    if (metrics == NULL || (metrics->totalBytesTransferred + bytes) <= metrics->totalBytesTransferred) {
      return;
    }

    metrics->totalBytesTransferred += bytes;
}

void serverMetricsRecordBytesReceived(server_metrics* metrics, size_t bytes) {

    if (metrics == NULL || (metrics->totalBytesReceived + bytes) <= metrics->totalBytesReceived) {
        return;
    }

    metrics->totalBytesReceived+= bytes;
}
