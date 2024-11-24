#ifndef GREETINGS_H
#define GREETINGS_H

#include "./core/selector.h"

void greetingOnArrival(const unsigned state, struct selector_key *key);
void greetingOnArrivalForManager(const unsigned state, struct selector_key *key);

#endif // GREETINGS_H
