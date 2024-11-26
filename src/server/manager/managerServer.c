#include "managerServer.h"
#include "../greetings.h"
#include "../core/transaction.h"
#include "../core/buffer.h"
#include "../core/stm.h"
#include "../core/buffer.h"
#include "../logging/auth.h"
#include "../client/pop3Parser.h"
#include "stdio.h"

#include "managerParser.h"
#include "../update.h"

#define BUFFER_SIZE 8192

//------------------------------------- Array de estados para la stm ---------------------------------------------------

static const struct state_definition stateHandlers[] = {
    {
        .state = MANAGER_GREETINGS,
        .on_arrival = greetingOnArrivalForManager,
        .on_write_ready = writeOnReadyManager
    },
    {
        .state = MANAGER_AUTHORIZATION,
        .on_arrival = authOnArrival,
        .on_read_ready = readOnReadyManager,
        .on_write_ready = writeOnReadyManager,
    },
    {
        .state = MANAGER_TRANSACTION,
        .on_arrival = transactionOnArrivalForManager,
        .on_read_ready = readOnReadyManager,
        .on_write_ready = writeOnReadyManager,
    },
    {
        .state = MANAGER_EXIT,
        .on_arrival = exitOnArrival,
        .on_write_ready = writeOnReadyManager,
    },{
        .state = MANAGER_DONE,
    },

    {
        .state = MANAGER_ERROR
    }
};

//----------------------------------- Struct to storage relevant info for the manager in selectorKey -------------------

managerData * newManagerData(const struct sockaddr_storage managerAddress) {

  managerData * managerData = calloc(1, sizeof(struct managerData));

  managerData->manager_data.stateMachine.initial = MANAGER_GREETINGS;
  managerData->manager_data.stateMachine.max_state = MANAGER_ERROR;
  managerData->manager_data.stateMachine.states = stateHandlers;
  stm_init(&managerData->manager_data.stateMachine);

  uint8_t* readBuffer = malloc(BUFFER_SIZE);
  uint8_t* writeBuffer = malloc(BUFFER_SIZE);
  buffer_init(&managerData->manager_data.readBuffer, BUFFER_SIZE, readBuffer);
  buffer_init(&managerData->manager_data.writeBuffer, BUFFER_SIZE, writeBuffer);
  const methodsMap * map = getManagerMethods();
  managerData->manager_data.parser = parserInit(map);
  managerData->manager_data.currentUsername = NULL;
  managerData->manager_data.isAuth = false;
  managerData->manager_data.closed = false;

  managerData->manager_data.sockaddrStorage = managerAddress;

  return managerData;
}

//------------------------------ Passive Socket ------------------------------------------------------------------------

void managerDone(struct selector_key* key) {
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

void managerRead(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm_manager st = stm_handler_read(stm, key);

  if (MANAGER_ERROR == st || MANAGER_DONE == st) {
    managerDone(key);
  }
}
void managerWrite(struct selector_key* key){
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm_manager st = stm_handler_write(stm, key);

  if (MANAGER_ERROR == st || MANAGER_DONE == st) {
    managerDone(key);
  }
}

void managerBlock(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  const enum states_from_stm_manager st = stm_handler_block(stm, key);

  if (MANAGER_ERROR == st || MANAGER_DONE == st) {
    managerDone(key);
  }
}
void managerClose(struct selector_key* key) {
  struct state_machine* stm = &ATTACHMENT_USER(key)->stateMachine;
  stm_handler_close(stm, key);
  managerDone(key);
}

static fd_handler handler = {
    .handle_read = managerRead,
    .handle_write = managerWrite,
    .handle_block = managerBlock,
    .handle_close = managerClose,
};


void managerPassiveAccept(struct selector_key* key) {

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

  if (SELECTOR_SUCCESS != selector_register(key->s, manager, &handler, OP_WRITE, managerData)) {
    goto fail;
  }
  return;

fail:
  if (manager != -1) {
    close(manager);
  }
}


//------------------------------------- Generic handler for MANAGER -------------------------

unsigned readOnReadyManager(struct selector_key * key) {
  managerData * data = ATTACHMENT_MANAGER(key);
  bool isFinished = readAndParse(key);
  if (isFinished) {
    unsigned next = UNKNOWN;
    switch (stm_state(&data->manager_data.stateMachine)) {
    case MANAGER_AUTHORIZATION:
      next = authOnReadReadyAdmin(key);
      break;
    case MANAGER_TRANSACTION:
      next = transactionManagerOnReadReady(key);
      break;
    }
    resetParser(data->manager_data.parser);
    selector_set_interest_key(key, OP_WRITE);
    return next;
  }
  return stm_state(&data->manager_data.stateMachine);
}

unsigned writeOnReadyManager(struct selector_key * key) {
  managerData * data = ATTACHMENT_MANAGER(key);
  bool isFinished = sendFromBuffer(key);
  if (isFinished) {
    unsigned next = UNKNOWN;
    switch (stm_state(&data->manager_data.stateMachine)) {
    case MANAGER_GREETINGS:
      next = MANAGER_AUTHORIZATION;
      break;
    case MANAGER_AUTHORIZATION:
      if (data->manager_data.isAuth)
        next = MANAGER_TRANSACTION;
      else
        next = MANAGER_AUTHORIZATION;
      break;
    case MANAGER_TRANSACTION:
      next = MANAGER_TRANSACTION;
      break;
    case MANAGER_EXIT:
      next = MANAGER_DONE;
    }

    if (!buffer_can_read(&data->manager_data.readBuffer)) {
      selector_set_interest_key(key, OP_READ);
      return next;
    }
    jump(&data->manager_data.stateMachine, next, key);
    selector_set_interest_key(key, OP_READ);
    readOnReadyManager(key);
  }
  return stm_state(&data->manager_data.stateMachine);
}


