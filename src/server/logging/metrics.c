#import "metrics.h"
#import <stdlib.h>
#import <string.h>

server_metrics *serverMetricsCreate(server_metrics* metrics) {

    server_metrics *serverMetrics = malloc(sizeof(server_metrics));
    memset(serverMetrics, 0, sizeof(server_metrics));

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
