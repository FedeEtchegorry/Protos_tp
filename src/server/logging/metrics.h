#ifndef METRICS_H
#define METRICS_H

#import <stdlib.h>

typedef struct {

  size_t historicConectionsCount;
  size_t currentConectionsCount;
  size_t totalBytesTransferred;
  size_t totalBytesReceived;

} server_metrics;

server_metrics *serverMetricsCreate(server_metrics* metrics);

void serverMetricsFree(server_metrics** metrics);

void serverMetricsRecordNewConection(server_metrics* metrics);

void serverMetricsRecordDropConection(server_metrics* metrics);

void serverMetricsRecordBytesTransferred(server_metrics* metrics, size_t bytes);

void serverMetricsRecordBytesReceived(server_metrics* metrics, size_t bytes);

#endif // METRICS_H