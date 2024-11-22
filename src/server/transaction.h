#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "selector.h"

typedef struct mailInfo {
    char * filename;
    unsigned size;
    bool deleted;
    bool seen;
} mailInfo;

void initTransactionModule(const char * mailDir);

void transactionOnArrival(const unsigned int state, struct selector_key *key);
unsigned transactionOnReadReady(struct selector_key *key);
unsigned transactionOnWriteReady(struct selector_key *key);

#endif //TRANSACTION_H
