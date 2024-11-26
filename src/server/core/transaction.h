#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "selector.h"

typedef struct mailInfo {
    char * filename;
    unsigned size;
    bool deleted;
    bool seen;
    long long unsigned int checksum;
} mailInfo;

void transactionOnArrival(const unsigned int state, struct selector_key *key);
unsigned transactionOnReadReady(struct selector_key *key);

void transactionOnArrivalForManager(const unsigned int state, struct selector_key* key);
unsigned transactionManagerOnReadReady(struct selector_key* key);

#endif //TRANSACTION_H
