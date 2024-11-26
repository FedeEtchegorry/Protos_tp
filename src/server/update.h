#ifndef UPDATE_H
#define UPDATE_H

#include "./core/selector.h"

void updateOnArrival(const unsigned int state, struct selector_key *key);

void exitOnArrival(const unsigned int state, struct selector_key *key);

void errorOnArrival(const unsigned int state, struct selector_key *key);
#endif // UPDATE_H
