#include "auth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "POP3Server.h"
#include "users.h"



//---------------------------------Private definitions-------------------------------
static void handleUsername(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    const char* username = data->pop3Parser.arg;
    if (username == NULL)
        username = "";

    if (data->currentUsername != NULL)
        free(data->currentUsername);
    data->currentUsername = strdup(username);

    writeInBuffer(key, true, false, NULL, 0);
}

static void handlePassword(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    if (data->currentUsername == NULL) {
        writeInBuffer(key, true, true, NO_USERNAME, sizeof(NO_USERNAME)-1);
        return;
    }

    const char* password = data->pop3Parser.arg;
    if(password == NULL)
        password = "";

    if(!userLogin(data->currentUsername, data->pop3Parser.arg)) {
        writeInBuffer(key, true, true, AUTH_FAILED, sizeof(AUTH_FAILED)-1);
        data->currentUsername = NULL;
        return;
    }

    data->isAuth=true;
    writeInBuffer(key, true, false, AUTH_SUCCESS, sizeof(AUTH_SUCCESS) - 1);
}

static void handleUnknown(struct selector_key* key) {
    writeInBuffer(key, true, true, INVALID_METHOD, sizeof(INVALID_METHOD) - 1);
}

//----------------------------------USER Handlers------------------------------------
void authOnArrival(const unsigned state, struct selector_key* key) {
    selector_set_interest_key(key, OP_READ);
}

unsigned authOnReadReady(struct selector_key* key) {
    if (readAndParse(key)) {
        clientData* data = ATTACHMENT(key);
        switch (data->pop3Parser.method) {
            case USER:
                handleUsername(key);
                break;
            case PASS:
                handlePassword(key);
                break;
            case QUIT:
                return DONE;
            default:
                handleUnknown(key);
        }
        selector_set_interest_key(key, OP_WRITE);
    }
    return AUTHORIZATION;
}

unsigned authOnWriteReady(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    if (data->isAuth)
        return TRANSACTION;

    selector_set_interest_key(key, OP_READ);
    return AUTHORIZATION;
}



