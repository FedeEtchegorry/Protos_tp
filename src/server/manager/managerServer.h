#ifndef PROTOS_TP_MANAGERSERVER_H
#define PROTOS_TP_MANAGERSERVER_H

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "../serverUtils.h"
#include "../core/selector.h"

#define SUCCESS_MSG "+OK "
#define ERROR_MSG   "-ERR "

//------------------------------------ Defines to be used by all modules -----------------------------------------------

#define ATTACHMENT_MANAGER(key) ((managerData *)(key->data))

#define GREETING_MANAGER "Manager server ready"
#define INVALID_COMMAND "Unknown command"
#define LOG_OUT "Logging out"
#define EXIT_MESSAGE "Connection closed"
#define UNEXPECTED_ERROR "Unexpected error"
#define AUTH_FAILED "Authentication failed"
#define AUTH_SUCCESS "Logged in successfully"
#define NO_USERNAME "No username given"
#define INVALID_METHOD "Invalid method"

typedef struct managerData {
    userData manager_data;
} managerData;


//--------------------------------------------- Manager States ---------------------------------------------------------

enum states_from_stm_manager {
  MANAGER_GREETINGS,
  MANAGER_AUTHORIZATION,
  MANAGER_TRANSACTION,
  MANAGER_EXIT,
  MANAGER_DONE,
  MANAGER_ERROR,
};

//--------------------------------------------- Public Functions -------------------------------------------------------

void managerPassiveAccept(struct selector_key* key);
unsigned writeOnReadyManager(struct selector_key * key);
unsigned readOnReadyManager(struct selector_key * key);
#endif // PROTOS_TP_MANAGERSERVER_H
