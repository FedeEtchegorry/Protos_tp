#include "transaction.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "POP3Server.h"

const char * mailDirectory = NULL;

//------------------------------------------------------Private Functions------------------------------------------
static unsigned handleList(struct selector_key * key) {
    
}
static unsigned handleRetr(struct selector_key * key) {

}
static unsigned handleRset(struct selector_key * key) {

}
static unsigned handleNoop(struct selector_key * key) {

}
static unsigned handleDelete(struct selector_key * key) {

}
static unsigned handleStat(struct selector_key * key) {

}
static unsigned handleQuit(struct selector_key * key) {

}
static unsigned handleUnknown(struct selector_key * key) {

}

//------------------------------------------------------Public Functions------------------------------------------
void transactionOnArrival(const unsigned int state, struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    resetParser(&data->pop3Parser);
    selector_set_interest_key(key, OP_READ);
}

unsigned transactionOnReadReady(struct selector_key* key) {
    if (readAndParse(key)) {
        clientData* data = ATTACHMENT(key);
        switch (data->pop3Parser.method) {
            case LIST:
                return handleList(key);
            case RETR:
                return handleRetr(key);
            case RSET:
                return handleRset(key);
            case NOOP:
                return handleNoop(key);
            case DELE:
                return handleDelete(key);
            case STAT:
                return handleStat(key);
            case QUIT:
                return handleQuit(key);
            default:
                return handleUnknown(key);
        }
    }
    return TRANSACTION;
}

unsigned transactionOnWriteReady(struct selector_key* key) {
    if (sendFromBuffer(key)) {
        clientData* data = ATTACHMENT(key);
        resetParser(&data->pop3Parser);
        selector_set_interest_key(key, OP_READ);
    }

    return TRANSACTION;
}

void initTransactionModule(const char * mailDir) {
    mailDirectory = strdup(mailDir);
}