#ifndef PROTOS_TP_SERVERUTILS_H
#define PROTOS_TP_SERVERUTILS_H

#include <stdbool.h>
#include <sys/socket.h>
#include "parserUtils.h"
#include "./core/stm.h"
#include "./core/buffer.h"

#define SUCCESS_MSG           "+OK "
#define ERROR_MSG             "-ERR "
#define ATTACHMENT_USER(key)  ((userData *)(key->data))

enum states_from_stm {
  GREETINGS,
  AUTHORIZATION,
  TRANSACTION,
  UPDATE,
  DONE,
  ERROR,
};

enum states_from_stm_manager {
  MANAGER_GREETINGS,
  MANAGER_AUTHORIZATION,
  MANAGER_TRANSACTION,
  MANAGER_DONE,
  MANAGER_ERROR,
};

typedef struct userData {

  struct state_machine stateMachine;
  struct sockaddr_storage sockaddrStorage;

  parser parser;
  buffer readBuffer;
  buffer writeBuffer;
  char * currentUsername;
  bool isAuth;
  bool closed;

} userData;

unsigned readOnReady(struct selector_key * key);
unsigned writeOnReady(struct selector_key * key);

unsigned readOnReadyManager(struct selector_key * key);
unsigned writeOnReadyManager(struct selector_key * key);

void writeInBuffer(struct selector_key* key, bool hasStatusCode, bool isError, char* msg, long len);
bool sendFromBuffer(struct selector_key* key);
bool readAndParse(struct selector_key* key);

fd_handler* getHandler();

#endif // PROTOS_TP_SERVERUTILS_H
