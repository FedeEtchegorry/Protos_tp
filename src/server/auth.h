#ifndef AUTH_H
#define AUTH_H

#include "selector.h"

void userOnArrival(const unsigned state, struct selector_key *key);
unsigned userOnReadReady(struct selector_key *key);
unsigned userOnWriteReady(struct selector_key *key);

void passOnArrival(const unsigned state, struct selector_key *key);
unsigned passOnReadReady(struct selector_key *key);
unsigned passOnWriteReady(struct selector_key *key);

#endif //AUTH_H
