#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serverUtils.h"
#include "./logging/auth.h"
#include "./client/pop3Parser.h"
#include "./client/pop3Server.h"


//------------------------------- Handlers for selector -----------------------------------------------

void server_done(struct selector_key* key) {
  userData * data = ATTACHMENT_USER(key);
  if (data->closed)
    return;
  data->closed = true;
  selector_unregister_fd(key->s, key->fd);
  close(key->fd);

  free(data->readBuffer.data);
  free(data->writeBuffer.data);
  free(data);
}

void pop3_read(struct selector_key*key){
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm st = stm_handler_read(stm, key);

  if (ERROR == st || DONE == st) {
    server_done(key);
  }
}

void manager_read(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm_manager st = stm_handler_read(stm, key);

  if (MANAGER_ERROR == st || MANAGER_DONE == st) {
    server_done(key);
  }
}


void pop3_write(struct selector_key* key){
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm st = stm_handler_write(stm, key);

  if (ERROR == st || DONE == st) {
    server_done(key);
  }
}

void manager_write(struct selector_key* key){
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm_manager st = stm_handler_write(stm, key);

  if (MANAGER_ERROR == st || MANAGER_DONE == st) {
    server_done(key);
  }
}

void pop3_block(struct selector_key* key){
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm st = stm_handler_block(stm, key);

  if (ERROR == st || DONE == st) {
    server_done(key);
  }
}

void manager_block(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm_manager st = stm_handler_block(stm, key);

  if (ERROR == st || DONE == st) {
    server_done(key);
  }
}
void server_close(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  stm_handler_close(stm, key);
  server_done(key);
}



void writeInBuffer(struct selector_key* key, bool hasStatusCode, bool isError, char* msg, long len) {
  userData * data = ATTACHMENT_USER(key);

  size_t writable;
  uint8_t* writeBuffer;

  if (hasStatusCode == true) {
    char* status = isError ? ERROR_MSG : SUCCESS_MSG;

    writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
    memcpy(writeBuffer, status, strlen(status));
    buffer_write_adv(&data->writeBuffer, strlen(status));
  }

  if (msg != NULL && len > 0) {
    writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
    memcpy(writeBuffer, msg, len);
    buffer_write_adv(&data->writeBuffer, len);
  }

  writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
  writeBuffer[0] = '\r';
  writeBuffer[1] = '\n';
  buffer_write_adv(&data->writeBuffer, 2);
}

bool sendFromBuffer(struct selector_key* key) {
  userData * data = ATTACHMENT_USER(key);

  size_t readable;
  uint8_t* readBuffer = buffer_read_ptr(&data->writeBuffer, &readable);

  ssize_t writeCount = send(key->fd, readBuffer, readable, 0);

  buffer_read_adv(&data->writeBuffer, writeCount);

  return readable == (size_t) writeCount;
}

bool readAndParse(struct selector_key* key) {
  userData * data = ATTACHMENT_USER(key);

  size_t readLimit;
  uint8_t* readBuffer = buffer_write_ptr(&data->readBuffer, &readLimit);
  const ssize_t readCount = recv(key->fd, readBuffer, readLimit, 0);
  buffer_write_adv(&data->readBuffer, readCount);

  while (!parserIsFinished(&data->parser) && buffer_can_read(&data->readBuffer))
    parse_feed(&data->parser, buffer_read(&data->readBuffer));

  return parserIsFinished(&data->parser);
}
