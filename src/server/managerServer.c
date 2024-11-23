#include "managerServer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "selector.h"

#include <sys/socket.h>

//#include "POP3Server.h"
#include "auth.h"
#include "buffer.h"
#include "greetings.h"
#include "pop3Parser.h"
#include "selector.h"
#include "stm.h"
#include "transaction.h"
#include "update.h"

#define BUFFER_SIZE 8192

#define SUCCESS_MSG "+OK "
#define ERROR_MSG "-ERR "

//-------------------------------------Generic handler (just to support pipelining)-------------------------
static unsigned readOnReady(struct selector_key * key) {
  managerData* data = ATTACHMENT_MANAGER(key);
  bool isFinished = readAndParse(key);
  if (isFinished) {
    printf("Entrando a read finished\n");
    unsigned next;
    switch (stm_state(&data->mngStateMachine)) {
    case AUTHORIZATION_MNG:
      next = authOnReadReady(key);
      break;
    case TRANSACTION_MNG:
      next= transactionOnReadReady(key);
      break;
    }
    resetParser(&data->pop3Parser);
    selector_set_interest_key(key, OP_WRITE);
    return next;
  }
  return stm_state(&data->mngStateMachine);
}

static unsigned writeOnReady(struct selector_key * key) {
  managerData * data = ATTACHMENT_MANAGER(key);
  bool isFinished = sendFromBuffer(key);
  if (isFinished) {
    unsigned next;
    switch (stm_state(&data->mngStateMachine)) {
    case GREETINGS_MNG:
      next = AUTHORIZATION_MNG;
      break;
    case AUTHORIZATION_MNG:
      if (data->isAuth)
        next = TRANSACTION_MNG;
      else
        next = AUTHORIZATION_MNG;
      break;
    case TRANSACTION_MNG:
      next = TRANSACTION_MNG;
      break;
    case UPDATE_MNG:
      next = DONE_MNG;
    }

    if (!buffer_can_read(&data->pop3Parser.buffer)) {
      printf("no quedo nada y retorno %d", next);
      selector_set_interest_key(key, OP_READ);
      return next;
    }

    printf("queda algo en el buffer y salto a %d", next);
    jump(&data->mngStateMachine, next, key);
    selector_set_interest_key(key, OP_READ);
    readOnReady(key);
  }
  return stm_state(&data->mngStateMachine);
}

//-------------------------------------Array de estados para la stm------------------------------------------
static const struct state_definition stateHandlers[] = {
    {
        .state = GREETINGS_MNG,
        .on_arrival = greetingOnArrivalForManager,
        .on_write_ready = writeOnReady
    },
    {
        .state = AUTHORIZATION_MNG,
        .on_arrival = authOnArrival,
        .on_read_ready = readOnReady,
        .on_write_ready = writeOnReady,
    },
    {
        .state = TRANSACTION_MNG,
        .on_arrival = transactionOnArrivalForManager,
        .on_read_ready = readOnReady,
        .on_write_ready = writeOnReady,
    },

    {
        .state = DONE_MNG,
    },
    {
        .state = ERROR_MNG
    }
};

//-----------------------------------Struct to storage relevant info for the manager in selectorKey---------------------
managerData * newManagerData(const struct sockaddr_storage managerAddress) {
  managerData * managerData = calloc(1, sizeof(struct managerData));

  managerData->mngStateMachine.initial = GREETINGS_MNG;
  managerData->mngStateMachine.max_state = ERROR_MNG;
  managerData->mngStateMachine.states = stateHandlers;
  stm_init(&managerData->mngStateMachine);

  uint8_t* readBuffer = malloc(BUFFER_SIZE);
  uint8_t* writeBuffer = malloc(BUFFER_SIZE);
  buffer_init(&managerData->readBuffer, BUFFER_SIZE, readBuffer);
  buffer_init(&managerData->writeBuffer, BUFFER_SIZE, writeBuffer);

  parserInit(&managerData->pop3Parser);

  managerData->currentUsername = NULL;
  managerData->isAuth = false;

  return managerData;
}

//-------------------------------Handlers for selector-----------------------------------------------
static void manager_done(struct selector_key* key) {
  managerData * data = ATTACHMENT_MANAGER(key);

  selector_unregister_fd(key->s, key->fd);
  close(key->fd);

  free(data->readBuffer.data);
  free(data->writeBuffer.data);
  free(data);
}

static void manager_read(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_MANAGER(key)->mngStateMachine;
  const enum manager_server_state st = stm_handler_read(stm, key);

  if (ERROR_MNG == st || DONE_MNG == st) {
    manager_done(key);
  }
}

static void manager_write(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_MANAGER(key)->mngStateMachine;
  const enum manager_server_state st = stm_handler_write(stm, key);

  if (ERROR_MNG == st || DONE_MNG == st) {
    manager_done(key);
  }
}

static void manager_block(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_MANAGER(key)->mngStateMachine;
  const enum manager_server_state st = stm_handler_block(stm, key);

  if (ERROR_MNG == st || DONE_MNG == st) {
    manager_done(key);
  }
}

static const fd_handler pop3_handler = {
    .handle_read = manager_read,
    .handle_write = manager_write,
    .handle_block = manager_block,
};

//------------------------------Passive Socket--------------------------------------------------------
void manager_passive_accept(struct selector_key* key) {
  struct sockaddr_storage manager_addr;
  socklen_t manager_addr_len = sizeof(manager_addr);

  const int manager = accept(key->fd, (struct sockaddr*)&manager_addr, &manager_addr_len);
  if (manager == -1) {
    goto fail;
  }

  if (selector_fd_set_nio(manager) == -1) {
    goto fail;
  }

  managerData * managerData = newManagerData(manager_addr);
  if (SELECTOR_SUCCESS != selector_register(key->s, manager, &pop3_handler, OP_WRITE, managerData)) {
    goto fail;
  }
  return;

fail:
  if (manager != -1) {
    close(manager);
  }
}

//
//void writeInBuffer(struct selector_key* key, bool hasStatusCode, bool isError, char* msg, long len) {
//  managerData * data = ATTACHMENT_MANAGER(key);
//
//  size_t writable;
//  uint8_t* writeBuffer;
//
//  if (hasStatusCode == true) {
//    char* status = isError ? ERROR_MSG : SUCCESS_MSG;
//
//    writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
//    memcpy(writeBuffer, status, strlen(status));
//    buffer_write_adv(&data->writeBuffer, strlen(status));
//  }
//
//  if (msg != NULL && len > 0) {
//    writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
//    memcpy(writeBuffer, msg, len);
//    buffer_write_adv(&data->writeBuffer, len);
//  }
//
//  writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);
//  writeBuffer[0] = '\r';
//  writeBuffer[1] = '\n';
//  buffer_write_adv(&data->writeBuffer, 2);
//}
//
//bool sendFromBuffer(struct selector_key* key) {
//  managerData * data = ATTACHMENT_MANAGER(key);
//
//  size_t readable;
//  uint8_t* readBuffer = buffer_read_ptr(&data->writeBuffer, &readable);
//
//  ssize_t writeCount = send(key->fd, readBuffer, readable, 0);
//
//  buffer_read_adv(&data->writeBuffer, writeCount);
//
//  return readable == writeCount;
//}
//
//
//
//bool readAndParse(struct selector_key* key) {
//  managerData * data = ATTACHMENT_MANAGER(key);
//
//  size_t readLimit;
//  uint8_t* readBuffer = buffer_write_ptr(&data->readBuffer, &readLimit);
//  const ssize_t readCount = recv(key->fd, readBuffer, readLimit, 0);
//  buffer_write_adv(&data->readBuffer, readCount);
//
//  while (!parserIsFinished(&data->pop3Parser) && buffer_can_read(&data->readBuffer))
//    parse_feed(&data->pop3Parser, buffer_read(&data->readBuffer));
//
//  if (parserIsFinished(&data->pop3Parser)) {
//    printf("terminooooo");
//  } else
//    printf("no terminooooo");
//  return parserIsFinished(&data->pop3Parser);
//}
