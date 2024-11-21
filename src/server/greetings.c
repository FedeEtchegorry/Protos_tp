#include "greetings.h"

#include <string.h>

#include "POP3Server.h"
#include "buffer.h"

#define GREETING "POP3 server ready\n"

void greetingOnArrival(const unsigned state, struct selector_key *key) {
    writeInBuffer(key, false, GREETING, sizeof(GREETING));
    selector_set_interest_key(key, OP_WRITE);
}

unsigned greetingOnWriteReady(struct selector_key *key) {
    if(!sendFromBuffer(key))
        return GREETINGS;

    return AUTH_USER;
}