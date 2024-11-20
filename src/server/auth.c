#include "auth.h"

#include <string.h>

#include "POP3Server.h"
#include "buffer.h"

#define SUCCESS "+AUTH\n"
#define ERROR "-ERR\n"

void userOnArrival(const unsigned state, struct selector_key *key) {
    clientData * data = ATTACHMENT(key);
    resetParser(&data->pop3Parser);
    selector_set_interest_key(key, OP_READ);
}


unsigned userOnReadReady(struct selector_key *key) {
    clientData* data = ATTACHMENT(key);

    size_t readLimit;
    uint8_t* readBuffer = buffer_write_ptr(&data->readBuffer, &readLimit);
    const ssize_t readCount = recv(key->fd, readBuffer, readLimit, 0);
    buffer_write_adv(&data->readBuffer, readCount);

    parse(&data->pop3Parser, &data->readBuffer);
    if (parserIsFinished(&data->pop3Parser)) {
        size_t writable;
        uint8_t* writeBuffer = buffer_write_ptr(&data->writeBuffer, &writable);

        char * msg;
        if(data->pop3Parser.method == USER)
            msg = SUCCESS;
        else
            msg = ERROR;

        memcpy(writeBuffer, msg, strlen(msg));

        buffer_write_adv(&data->writeBuffer, strlen(msg));
        selector_set_interest_key(key, OP_WRITE);
    }

    return AUTH_USER;
}

unsigned userOnWriteReady(struct selector_key *key) {
    clientData * data = ATTACHMENT(key);

    size_t readable;
    uint8_t * readBuffer = buffer_read_ptr(&data->writeBuffer, &readable);

    ssize_t writeCount = send(key->fd, readBuffer, readable, 0);

    buffer_read_adv(&data->writeBuffer, writeCount);
    if(buffer_can_read(&data->writeBuffer))
        return AUTH_USER;

    return AUTH_PASS;
}

void passOnArrival(const unsigned state, struct selector_key *key) {
    clientData * data = ATTACHMENT(key);
    resetParser(&data->pop3Parser);
    selector_set_interest_key(key, OP_READ);
}

unsigned passOnReadReady(struct selector_key *key) {
    return AUTH_PASS;
}
unsigned passOnWriteReady(struct selector_key *key) {
    return AUTH_PASS;
}

