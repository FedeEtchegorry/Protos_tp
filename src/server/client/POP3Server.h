#ifndef POP3STATEMACHINE_H
#define POP3STATEMACHINE_H

#include "../serverUtils.h"
#include "../core/selector.h"
#include "../core/transaction.h"

#define SUCCESS_MSG "+OK "
#define ERROR_MSG   "-ERR "

//------------------------------------ Defines to be used by all modules -----------------------------------------------

#define ATTACHMENT(key) ((clientData *)(key->data))

#define GREETING "POP3 server ready"
#define NO_MESSAGE_FOUND "No message found"
#define INVALID_NUMBER "Invalid message number"
#define INVALID_COMMAND "Unknown command"
#define LOG_OUT "Logging out"
#define AUTH_FAILED "Authentication failed"
#define AUTH_SUCCESS "Logged in successfully"
#define NO_USERNAME "No username given"
#define INVALID_METHOD "Invalid method"
#define MESSAGE_DELETED "Message deleted"

#define MAX_MAILS   50

extern char * mailDirectory;

typedef struct clientData {
    struct userData data;
    struct mailInfo * mails[MAX_MAILS];
    unsigned mailCount;
} clientData;

//--------------------------------------------- Public Functions -------------------------------------------------------

void initMaildir(const char * directory);
void pop3_passive_accept(struct selector_key* key);

#endif // POP3STATEMACHINE_H
