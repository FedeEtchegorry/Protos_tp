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


void writeInBuffer(struct selector_key* key, bool hasStatusCode, bool isError, char* msg, long len);
bool sendFromBuffer(struct selector_key* key);
bool readAndParse(struct selector_key* key);

void server_done(struct selector_key* key);
void pop3_read(struct selector_key*key);
void manager_read(struct selector_key* key);
void pop3_write(struct selector_key* key);
void manager_write(struct selector_key* key);
void pop3_block(struct selector_key* key);
void manager_block(struct selector_key* key);
void server_close(struct selector_key* key);

fd_handler* getHandlerForClient();
fd_handler* getHandlerForManager();


fd_handler* getHandlerForClient();
fd_handler* getHandlerForManager();

#endif // PROTOS_TP_SERVERUTILS_H
