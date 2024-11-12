#ifndef ECHOSERVER_H
#define ECHOSERVER_H

void passive_accept(struct selector_key *key);
void pool_destroy(void);
int handleEchoClient(int clientFd);


#endif //ECHOSERVER_H
