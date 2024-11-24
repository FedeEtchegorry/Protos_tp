#include "greetings.h"
#include <stdio.h>
#include <string.h>
#include "./core/buffer.h"
#include "./client/pop3Server.h"
#include "./manager/managerServer.h"

void greetingOnArrival(const unsigned state, struct selector_key *key) {
    writeInBuffer(key, true, false, GREETING, sizeof(GREETING));
    selector_set_interest_key(key, OP_WRITE);
}

void greetingOnArrivalForManager(const unsigned state, struct selector_key *key){
    writeInBuffer(key, true, false, GREETING_MANAGER, sizeof(GREETING_MANAGER));
    selector_set_interest_key(key, OP_WRITE);
}