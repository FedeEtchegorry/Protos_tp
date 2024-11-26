#ifndef PROTOS_TP_SERVERUTILS_H
#define PROTOS_TP_SERVERUTILS_H

#include <stdbool.h>
#include <sys/socket.h>
#include "parserUtils.h"
#include "./core/stm.h"
#include "./core/buffer.h"

#define ATTACHMENT_USER(key)  ((userData *)(key->data))

typedef struct userData {

  struct state_machine stateMachine;
  struct sockaddr_storage sockaddrStorage;

  parserADT parser;
  buffer readBuffer;
  buffer writeBuffer;
  char * currentUsername;
  bool isAuth;
  bool closed;
  bool isEmailFinished;
} userData;

void partialWriteInBuffer(struct selector_key* key, char* msg, long len);
void writeInBuffer(struct selector_key* key, bool hasStatusCode, bool isError, char* msg, long len);
bool sendFromBuffer(struct selector_key* key);
bool readAndParse(struct selector_key* key);




#endif // PROTOS_TP_SERVERUTILS_H
