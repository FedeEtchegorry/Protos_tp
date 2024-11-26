#ifndef POP3STATEMACHINE_H
#define POP3STATEMACHINE_H

#include "../serverConfigs.h"
#include "../serverUtils.h"
#include "../core/selector.h"
#include "../core/transaction.h"
#include "../args.h"

#define ATTACHMENT(key) ((clientData *)(key->data))

typedef struct pop3Config {
    const char* maildir;
    bool transformation_enabled;
    struct users* users;
    unsigned nusers;
} pop3Config;

extern char* mailDirectory;

typedef struct clientData {
    struct userData data;
    struct mailInfo* mails[MAX_MAILS];
    unsigned mailCount;
} clientData;

enum states_from_stm {
    GREETINGS,
    AUTHORIZATION,
    TRANSACTION,
    TRANSFORMATION,
    UPDATE,
    EXIT,
    DONE,
    ERROR,
};


//--------------------------------------------- Public Functions -------------------------------------------------------

void initPOP3Config(struct pop3Config config);
void pop3PassiveAccept(struct selector_key* key);
unsigned writeOnReadyPop3(struct selector_key* key);
unsigned readOnReadyPop3(struct selector_key* key);

#endif // POP3STATEMACHINE_H
