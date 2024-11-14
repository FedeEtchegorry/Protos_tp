#ifndef ECHOSERVER_H
#define ECHOSERVER_H

#include "selector.h"

void passive_accept(const struct selector_key *key);
void pool_destroy(void);
int handleEchoClient(int clientFd);


#endif //ECHOSERVER_H
