#include "auth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../core/users.h"
#include "../client/pop3Server.h"
#include "../client/pop3Parser.h"
#include "../manager/managerParser.h"
#include "../manager/managerServer.h"

//--------------------------------- Private Functions -------------------------------

static void handleUsername(struct selector_key* key) {
    userData * data = ATTACHMENT_USER(key);
    const char* username = data->parser.arg;
    if (username == NULL)
        username = "";

    if (data->currentUsername != NULL)
        free(data->currentUsername);
    data->currentUsername = strdup(username);

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
static void handlePasswordAdmin(struct selector_key* key) {
    managerData * data = ATTACHMENT_MANAGER(key);
    if (data->manager_data.currentUsername == NULL) {
        writeInBuffer(key, true, true, NO_USERNAME, sizeof(NO_USERNAME)-1);
        return;
    }

    const char* password = data->manager_data.parser.arg;
    if(password == NULL)
        password = "";



    if(!userLoginAdmin(data->manager_data.currentUsername, data->manager_data.parser.arg)) {
        writeInBuffer(key, true, true, AUTH_FAILED, sizeof(AUTH_FAILED)-1);
        data->manager_data.currentUsername = NULL;
        return;
    }

    data->manager_data.isAuth=true;
    writeInBuffer(key, true, false, AUTH_SUCCESS, sizeof(AUTH_SUCCESS) - 1);
}

static void handleUnknown(struct selector_key* key) {
    writeInBuffer(key, true, true, INVALID_METHOD, sizeof(INVALID_METHOD) - 1);
}

//---------------------------------- USER Handlers ------------------------------------

void authOnArrival(const unsigned state, struct selector_key* key) {
    selector_set_interest_key(key, OP_READ);
}

unsigned authOnReadReady(struct selector_key* key) {
    userData * data = ATTACHMENT_USER(key);
    switch (data->parser.method) {
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
    return AUTHORIZATION;
}

unsigned authOnReadReadyAdmin(struct selector_key* key) {
    userData * data = ATTACHMENT_USER(key);
    switch (data->parser.method) {
    case USER_M:
            handleUsername(key);
            break;
    case PASS_M:
            handlePasswordAdmin(key);
            break;
    case QUIT_M:
            return MANAGER_DONE;
    default:
            handleUnknown(key);
    }
    return MANAGER_AUTHORIZATION;
}