#ifndef POP3STATEMACHINE_H
#define POP3STATEMACHINE_H

#include <sys/socket.h>

#include "pop3Parser.h"
#include "buffer.h"
#include "stm.h"

#define ATTACHMENT(key) ((clientData *)(key->data))

typedef struct clientData {
    struct state_machine stateMachine;

    pop3Parser pop3Parser;

    struct sockaddr_storage clientAddress;

    bool isAuth;

    buffer readBuffer;
    buffer writeBuffer;
} clientData;


//---------------------------------------------POP3 States-----------------------------------------------------
enum pop3_state {
    GREETINGS,

    AUTHORIZATION,

    TRANSACTION,

    DONE,
    ERROR,
};

void pop3_passive_accept(struct selector_key* key);
void writeInBuffer(struct selector_key * key, bool isError, char * msg, long len);
bool sendFromBuffer(struct selector_key * key);
bool readAndParse(struct selector_key * key);

#endif //POP3STATEMACHINE_H
