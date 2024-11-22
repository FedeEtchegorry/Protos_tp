#ifndef POP3STATEMACHINE_H
#define POP3STATEMACHINE_H

#include <sys/socket.h>

#include "transaction.h"
#include "pop3Parser.h"
#include "buffer.h"
#include "stm.h"

//------------------------------------Defines to be used by all modules-----------------------------------
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

#define MAX_MAILS 50

extern char * mailDirectory;

typedef struct clientData {
    struct state_machine stateMachine;

    pop3Parser pop3Parser;

    struct sockaddr_storage clientAddress;

    char * currentUsername;
    bool isAuth;

    struct mailInfo * mails[MAX_MAILS];
    unsigned mailCount;

    buffer readBuffer;
    buffer writeBuffer;
} clientData;


//---------------------------------------------POP3 States-----------------------------------------------------
enum pop3_state {
    GREETINGS,

    AUTHORIZATION,

    TRANSACTION,

    UPDATE,

    DONE,
    ERROR,
};

//---------------------------------------------Public Functions------------------------------------------------
void initMaildir(const char * directory);
void pop3_passive_accept(struct selector_key* key);
void writeInBuffer(struct selector_key * key, bool hasStatusCode, bool isError, char * msg, long len);
bool sendFromBuffer(struct selector_key * key);
bool readAndParse(struct selector_key * key);

#endif //POP3STATEMACHINE_H
