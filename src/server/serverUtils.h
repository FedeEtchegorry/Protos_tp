#ifndef PROTOS_TP_SERVERUTILS_H
#define PROTOS_TP_SERVERUTILS_H

#include "pop3Parser.h"
#include "stm.h"
#include <bits/socket.h>

#define ATTACHMENT_USER(key) ((userData *)(key->data))


enum states_from_stm {
  GREETINGS,
  AUTHORIZATION,
  TRANSACTION,
  UPDATE,
  DONE,
  ERROR,
};


typedef struct userData {
  struct state_machine stateMachine;

  parser parser;

  struct sockaddr_storage sockaddrStorage;

  char * currentUsername;
  bool isAuth;
  bool closed;

  buffer readBuffer;
  buffer writeBuffer;
} userData;


#include "serverUtils.h"
#include "POP3Server.h"
#include "auth.h"
#include "buffer.h"
#include "greetings.h"
#include "pop3Parser.h"
#include "stm.h"
#include "transaction.h"
#include "update.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>



#define BUFFER_SIZE 8192

#define SUCCESS_MSG "+OK "
#define ERROR_MSG "-ERR "




unsigned readOnReady(struct selector_key * key);
unsigned writeOnReady(struct selector_key * key);

void writeInBuffer(struct selector_key* key, bool hasStatusCode, bool isError, char* msg, long len);
bool sendFromBuffer(struct selector_key* key);
bool readAndParse(struct selector_key* key);

fd_handler* getHandler();



#endif // PROTOS_TP_SERVERUTILS_H
