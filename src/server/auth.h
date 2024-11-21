#ifndef AUTH_H
#define AUTH_H

#include "selector.h"

void authOnArrival(const unsigned state, struct selector_key *key);
unsigned authOnReadReady(struct selector_key *key);
unsigned authOnWriteReady(struct selector_key *key);

#endif //AUTH_H
