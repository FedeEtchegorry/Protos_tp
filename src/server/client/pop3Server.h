#ifndef POP3STATEMACHINE_H
#define POP3STATEMACHINE_H

#include "../serverConfigs.h"
#include "../serverUtils.h"
#include "../core/selector.h"
#include "../core/transaction.h"

#define ATTACHMENT(key) ((clientData *)(key->data))

extern char * mailDirectory;

typedef struct clientData {
    struct userData data;
    struct mailInfo *mails[MAX_MAILS];
    unsigned mailCount;
} clientData;

//--------------------------------------------- Public Functions -------------------------------------------------------

void initMaildir(const char * directory);
void pop3PassiveAccept(struct selector_key* key);
void freeClientData(struct clientData** clientData);
unsigned writeOnReadyPop3(struct selector_key * key);
unsigned readOnReadyPop3(struct selector_key * key);

#endif // POP3STATEMACHINE_H
