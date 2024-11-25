#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serverUtils.h"
#include "./logging/auth.h"
#include "./client/pop3Parser.h"
#include "./client/pop3Server.h"

//------------------------------------- Generic handler (just to support pipelining)  -------------------------

unsigned readOnReady(struct selector_key * key) {
  userData * data = ATTACHMENT_USER(key);
  bool isFinished = readAndParse(key);
  if (isFinished) {
    printf("Entrando a read finished\n");
    unsigned next = UNKNOWN;
    switch (stm_state(&data->stateMachine)) {
    case AUTHORIZATION:
      next = authOnReadReady(key);
      break;
    case TRANSACTION:
      next= transactionOnReadReady(key);
      break;
    }
    resetParser(&data->parser);
    selector_set_interest_key(key, OP_WRITE);
    return next;
  }
  return stm_state(&data->stateMachine);
}

unsigned writeOnReady(struct selector_key * key) {
  userData * data = ATTACHMENT_USER(key);
  bool isFinished = sendFromBuffer(key);
  if (isFinished) {
    unsigned next = UNKNOWN;
    switch (stm_state(&data->stateMachine)) {
    case GREETINGS:
      next = AUTHORIZATION;
      break;
    case AUTHORIZATION:
      if (data->isAuth)
        next = TRANSACTION;
      else
        next = AUTHORIZATION;
      break;
    case TRANSACTION:
      next = TRANSACTION;
      break;
    case UPDATE:
      next = DONE;
    }

    if (!buffer_can_read(&data->readBuffer)) {
      selector_set_interest_key(key, OP_READ);
      return next;
    }

    printf("queda algo en el buffer y salto a %d", next);
    jump(&data->stateMachine, next, key);
    selector_set_interest_key(key, OP_READ);
    readOnReady(key);
  }
  return stm_state(&data->stateMachine);
}

//------------------------------------- Generic handler for MANAGER -------------------------

unsigned readOnReadyManager(struct selector_key * key) {
  userData * data = ATTACHMENT_USER(key);
  bool isFinished = readAndParse(key);
  if (isFinished) {
    printf("Entrando a read finished de MANAGER\n");
    unsigned next = UNKNOWN;
    switch (stm_state(&data->stateMachine)) {
    case MANAGER_AUTHORIZATION:
      next = authOnReadReadyAdmin(key);
      break;
    case MANAGER_TRANSACTION:
      next = transactionManagerOnReadReady(key);
      break;
    }
    resetParser(&data->parser);
    selector_set_interest_key(key, OP_WRITE);
    return next;
  }
  return stm_state(&data->stateMachine);
}

unsigned writeOnReadyManager(struct selector_key * key) {
  userData * data = ATTACHMENT_USER(key);
  bool isFinished = sendFromBuffer(key);
  if (isFinished) {
    unsigned next = UNKNOWN;
    switch (stm_state(&data->stateMachine)) {
    case MANAGER_GREETINGS:
      next = MANAGER_AUTHORIZATION;
      break;
    case MANAGER_AUTHORIZATION:
      if (data->isAuth)
        next = MANAGER_TRANSACTION;
      else
        next = MANAGER_AUTHORIZATION;
      break;
    case MANAGER_TRANSACTION:
      next = MANAGER_TRANSACTION;
      break;
    }

    if (!buffer_can_read(&data->readBuffer)) {
      selector_set_interest_key(key, OP_READ);
      return next;
    }

    printf("queda algo en el buffer y salto a %d", next);
    jump(&data->stateMachine, next, key);
    selector_set_interest_key(key, OP_READ);
    readOnReady(key);
  }
  return stm_state(&data->stateMachine);
}

//------------------------------- Handlers for selector -----------------------------------------------

static void server_done(struct selector_key* key) {
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

static void server_read(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm st = stm_handler_read(stm, key);

  if (ERROR == st || DONE == st) {
    server_done(key);
  }
}

static void server_write(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm st = stm_handler_write(stm, key);

  if (ERROR == st || DONE == st) {
    server_done(key);
  }
}

static void server_block(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm st = stm_handler_block(stm, key);

  if (ERROR == st || DONE == st) {
    server_done(key);
  }
}
static void server_close(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  stm_handler_close(stm, key);
  server_done(key);
}

static fd_handler handler = {
    .handle_read = server_read,
    .handle_write = server_write,
    .handle_block = server_block,
    .handle_close = server_close,
};

fd_handler* getHandler(){
    return &handler;
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
