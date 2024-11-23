#ifndef PROTOS_TP_MANAGERSERVER_H
#define PROTOS_TP_MANAGERSERVER_H


#include <sys/socket.h>

#include "transaction.h"
#include "pop3Parser.h"
#include "buffer.h"
#include "stm.h"

//------------------------------------Defines to be used by all modules-----------------------------------
#define ATTACHMENT_MANAGER(key) ((managerData *)(key->data))

#define GREETING_MANAGER "Manager server ready"
#define INVALID_COMMAND "Unknown command"
#define LOG_OUT "Logging out"
#define AUTH_FAILED "Authentication failed"
#define AUTH_SUCCESS "Logged in successfully"
#define NO_USERNAME "No username given"
#define INVALID_METHOD "Invalid method"





typedef struct managerData {
  struct state_machine mngStateMachine;

  pop3Parser pop3Parser;

  struct sockaddr_storage managerAddress;

  char * currentUsername;
  bool isAuth;

  buffer readBuffer;
  buffer writeBuffer;
} managerData;


//---------------------------------------------Manager States-----------------------------------------------------
enum manager_server_state {
  GREETINGS_MNG,
  AUTHORIZATION_MNG,
  TRANSACTION_MNG,
  DONE_MNG,
  ERROR_MNG,
  UPDATE_MNG
};

//---------------------------------------------Public Functions------------------------------------------------
void manager_passive_accept(struct selector_key* key);
void writeInBuffer(struct selector_key * key, bool hasStatusCode, bool isError, char * msg, long len);
bool sendFromBuffer(struct selector_key * key);
bool readAndParse(struct selector_key * key);

#endif // PROTOS_TP_MANAGERSERVER_H
