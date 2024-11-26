#include "auth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../core/users.h"
#include "../client/pop3Server.h"
#include "../client/pop3Parser.h"
#include "../manager/managerParser.h"
#include "../manager/managerServer.h"

#define MAX_AUX_BUFFER_SIZE    255
//--------------------------------- Private Functions -------------------------------
static void handleCapa(struct selector_key* key){
  char method[MAX_AUX_BUFFER_SIZE];
  snprintf(method, MAX_AUX_BUFFER_SIZE, "%s", "Capability list follows");
  writeInBuffer(key, true, false, method, strlen(method));

  snprintf(method, MAX_AUX_BUFFER_SIZE, "%s", "CAPA");
  writeInBuffer(key, false, false, method, strlen(method));
  snprintf(method, MAX_AUX_BUFFER_SIZE, "%s", "USER");
  writeInBuffer(key, false, false, method, strlen(method));
  snprintf(method, MAX_AUX_BUFFER_SIZE, "%s", "UIDL");
  writeInBuffer(key, false, false, method, strlen(method));
  snprintf(method, MAX_AUX_BUFFER_SIZE, "%s", "PIPELINING");
  writeInBuffer(key, false, false, method, strlen(method));
  writeInBuffer(key, false, false, ".", strlen("."));
}

static void handleUsername(struct selector_key* key) {
    userData * data = ATTACHMENT_USER(key);
    const char* username = parserGetFirstArg(data->parser);
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

    const char* password = parserGetFirstArg(data->data.parser);
    if(password == NULL)
        password = "";

    if(!userLogin(data->data.currentUsername, password)) {
        writeInBuffer(key, true, true, AUTH_FAILED, sizeof(AUTH_FAILED)-1);
        data->data.currentUsername = NULL;
        return;
    }

    if(isConnected(data->data.currentUsername))
    {
        writeInBuffer(key, true, true, USER_ALREADY_CONNECTED, sizeof(USER_ALREADY_CONNECTED)-1);
        return;
    }

    data->data.isAuth=true;
    userConnected(data->data.currentUsername);
    writeInBuffer(key, true, false, AUTH_SUCCESS, sizeof(AUTH_SUCCESS) - 1);
}
static void handlePasswordAdmin(struct selector_key* key) {
    managerData * data = ATTACHMENT_MANAGER(key);
    if (data->manager_data.currentUsername == NULL) {
        writeInBuffer(key, true, true, NO_USERNAME, sizeof(NO_USERNAME)-1);
        return;
    }

    const char* password = parserGetFirstArg(data->manager_data.parser);
    if(password == NULL)
        password = "";



    if(!userLoginAdmin(data->manager_data.currentUsername, password)) {
        writeInBuffer(key, true, true, AUTH_FAILED, sizeof(AUTH_FAILED)-1);
        data->manager_data.currentUsername = NULL;
        return;
    }

    if(isConnected(data->manager_data.currentUsername))
    {
        writeInBuffer(key, true, true, USER_ALREADY_CONNECTED, sizeof(USER_ALREADY_CONNECTED)-1);
        return;
    }

    data->manager_data.isAuth=true;
    userConnected(data->manager_data.currentUsername);
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
    switch (parserGetMethod(data->parser)) {
        case USER:
            handleUsername(key);
            break;
        case PASS:
            handlePassword(key);
            break;
        case QUIT:
            return UPDATE;
        case CAPA:
            handleCapa(key);
            break;
        default:
            handleUnknown(key);
    }
    return AUTHORIZATION;
}

unsigned authOnReadReadyAdmin(struct selector_key* key) {
    userData * data = ATTACHMENT_USER(key);
    switch (parserGetMethod(data->parser)) {
    case USER_M:
            handleUsername(key);
            break;
    case PASS_M:
            handlePasswordAdmin(key);
            break;
    case CAPA_M:
            handleCapa(key);
            break;
    case QUIT_M:
            return EXIT;
    default:
            handleUnknown(key);
    }
    return MANAGER_AUTHORIZATION;
}