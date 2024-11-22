#include "greetings.h"

#include <stdio.h>
#include <string.h>

#include "POP3Server.h"
#include "buffer.h"


void greetingOnArrival(const unsigned state, struct selector_key *key) {
    writeInBuffer(key, true, false, GREETING, sizeof(GREETING));
    selector_set_interest_key(key, OP_WRITE);
}

