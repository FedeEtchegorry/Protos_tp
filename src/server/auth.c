#include "auth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "POP3Server.h"
#include "pop3Parser.h"
#include "managerParser.h"
#include "users.h"



//---------------------------------Private definitions-------------------------------
static void handleUsername(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    const char* username = data->data.parser.arg;
    if (username == NULL)
        username = "";

    if (data->data.currentUsername != NULL)
        free(data->data.currentUsername);
    data->data.currentUsername = strdup(username);

    writeInBuffer(key, true, false, NULL, 0);
}

static void handlePassword(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    if (data->data.currentUsername == NULL) {
        writeInBuffer(key, true, true, NO_USERNAME, sizeof(NO_USERNAME)-1);
        return;
    }

    const char* password = data->data.parser.arg;
    if(password == NULL)
        password = "";

    if(!userLogin(data->data.currentUsername, data->data.parser.arg)) {
        writeInBuffer(key, true, true, AUTH_FAILED, sizeof(AUTH_FAILED)-1);
        data->data.currentUsername = NULL;
        return;
    }

    data->data.isAuth=true;
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
    clientData* data = ATTACHMENT(key);
    switch (data->data.parser.method) {
        case USER | USER_M:
            handleUsername(key);
            break;
        case PASS | PASS_M:
            handlePassword(key);
            break;
        case QUIT | QUIT_M:
            return DONE;
        default:
            handleUnknown(key);
    }
    return AUTHORIZATION;
}





