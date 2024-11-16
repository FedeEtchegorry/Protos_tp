#ifndef POP3STATEMACHINE_H
#define POP3STATEMACHINE_H

#include <sys/socket.h>

#include "buffer.h"
#include "stm.h"
#include "parser.h"

#define ATTACHMENT(key) ((clientData *)(key->data))

typedef struct clientData {
    struct state_machine stateMachine;

    union {
        struct parser * authParser;
    } parsers;

    struct sockaddr_storage clientAddress;

    bool isAuth;

    buffer readBuffer;
    buffer writeBuffer;
} clientData;



#endif //POP3STATEMACHINE_H
