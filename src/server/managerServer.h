#ifndef PROTOS_TP_MANAGERSERVER_H
#define PROTOS_TP_MANAGERSERVER_H


#include <sys/socket.h>

#include "transaction.h"

#include "buffer.h"
#include "stm.h"
#include "serverUtils.h"

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
  struct userData manager_data;
} managerData;


////---------------------------------------------Manager States-----------------------------------------------------
//enum manager_server_state {
//  GREETINGS_MNG,
//  AUTHORIZATION_MNG,
//  TRANSACTION_MNG,
//  DONE_MNG,
//  ERROR_MNG,
//  UPDATE_MNG
//};

//---------------------------------------------Public Functions------------------------------------------------
void manager_passive_accept(struct selector_key* key);


#endif // PROTOS_TP_MANAGERSERVER_H