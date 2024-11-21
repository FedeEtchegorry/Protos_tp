#include "greetings.h"

#include <stdio.h>
#include <string.h>

#include "POP3Server.h"
#include "buffer.h"

#define GREETING "POP3 server ready"

void greetingOnArrival(const unsigned state, struct selector_key *key) {
    printf("Entre a greeting\n");
    writeInBuffer(key, false, GREETING, sizeof(GREETING));
    selector_set_interest_key(key, OP_WRITE);
}

unsigned greetingOnWriteReady(struct selector_key *key) {
    if(!sendFromBuffer(key))
        return GREETINGS;

    return AUTHORIZATION;
}