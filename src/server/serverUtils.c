#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serverUtils.h"
#include "./logging/auth.h"
#include "./client/pop3Parser.h"
#include "./client/pop3Server.h"
#include "./logging/metrics.h"

extern server_metrics *clientMetrics;

//------------------------------- Handlers for selector -----------------------------------------------

void writeInBuffer(struct selector_key* key, bool hasStatusCode, bool isError, char* msg, long len) {
  userData * data = ATTACHMENT_USER(key);

  size_t writable;
  uint8_t* writeBuffer;

  if (hasStatusCode == true) {
    char* status = isError ? ERROR_MSG : SUCCESS_MSG;

    writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
    memcpy(writeBuffer, status, strlen(status));
    buffer_write_adv(&data->writeBuffer, strlen(status));
    serverMetricsRecordBytesTransferred(clientMetrics, strlen(status));
  }

  if (msg != NULL && len > 0) {
    writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
    memcpy(writeBuffer, msg, len);
    buffer_write_adv(&data->writeBuffer, len);
    serverMetricsRecordBytesTransferred(clientMetrics, len);
  }

  writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
  writeBuffer[0] = '\r';
  writeBuffer[1] = '\n';
  buffer_write_adv(&data->writeBuffer, 2);
  serverMetricsRecordBytesTransferred(clientMetrics, 2);
}

bool sendFromBuffer(struct selector_key* key) {
  userData * data = ATTACHMENT_USER(key);

  size_t readable;
  uint8_t* readBuffer = buffer_read_ptr(&data->writeBuffer, &readable);

  ssize_t writeCount = send(key->fd, readBuffer, readable, 0);

  buffer_read_adv(&data->writeBuffer, writeCount);

  serverMetricsRecordBytesTransferred(clientMetrics, writeCount);

  return readable == (size_t) writeCount;
}

bool readAndParse(struct selector_key* key) {
  userData * data = ATTACHMENT_USER(key);

  size_t readLimit;
  uint8_t* readBuffer = buffer_write_ptr(&data->readBuffer, &readLimit);
  const ssize_t readCount = recv(key->fd, readBuffer, readLimit, 0);
  buffer_write_adv(&data->readBuffer, readCount);

  serverMetricsRecordBytesReceived(clientMetrics, readCount);

  while (!parserIsFinished(&data->parser) && buffer_can_read(&data->readBuffer))
    parse_feed(&data->parser, buffer_read(&data->readBuffer));

  return parserIsFinished(&data->parser);
}
