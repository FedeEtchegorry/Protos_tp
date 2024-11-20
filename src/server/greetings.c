#include "greetings.h"

#include <string.h>

#include "POP3Server.h"
#include "buffer.h"

#define GREETING "POP3Server 0.1\n"

void greetingOnArrival(const unsigned state, struct selector_key *key) {
    clientData * data = ATTACHMENT(key);
    size_t writable;
    uint8_t * writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);

    memcpy(writeBuffer, GREETING, sizeof(GREETING) - 1);

    buffer_write_adv(&data->writeBuffer, sizeof(GREETING) - 1);
    selector_set_interest_key(key, OP_WRITE);
}

unsigned greetingOnWriteReady(struct selector_key *key) {
    clientData * data = ATTACHMENT(key);

    size_t readable;
    uint8_t * readBuffer = buffer_read_ptr(&data->writeBuffer, &readable);

    ssize_t writeCount = send(key->fd, readBuffer, readable, 0);

    buffer_read_adv(&data->writeBuffer, writeCount);
    if(buffer_can_read(&data->writeBuffer))
        return GREETINGS;

    selector_set_interest_key(key, OP_READ);
    return AUTH_USER;
}