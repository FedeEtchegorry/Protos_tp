#ifndef METRICS_H
#define METRICS_H

#include <unistd.h>
#include "../core/users.h"

typedef struct {
  size_t totalCountConnections;
  size_t currentConectionsCount;
  size_t totalTransferredBytes;
  size_t totalReceivedBytes;
  size_t totalCountUsers;
  const size_t *ioReadBufferSize;
  const size_t *ioWriteBufferSize;
  char *dataFilePath;
} server_metrics;

server_metrics *serverMetricsCreate(char *dataFilePath, const size_t *ioReadBufferSize, const size_t *ioWriteBufferSize);

void serverMetricsFree(server_metrics** metrics);

void serverMetricsRecordNewUser(server_metrics* metrics);

void serverMetricsRecordNewConection(server_metrics* metrics);

void serverMetricsRecordDropConection(server_metrics* metrics);

void serverMetricsRecordBytesTransferred(server_metrics* metrics, size_t bytes);

void serverMetricsRecordBytesReceived(server_metrics* metrics, size_t bytes);

int serverMetricsRecordInFile(server_metrics *metrics);

#endif // METRICS_H