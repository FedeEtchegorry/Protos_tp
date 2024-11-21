#include "auth.h"

#include <stdio.h>
#include <string.h>

#include "POP3Server.h"
#include "users.h"

#define VALID_USER "Valid user"
#define AUTH_FAILED "Authentication failed"
#define AUTH_SUCCESS "Logged in successfully"
#define NO_USERNAME "No username given"
#define INVALID_METHOD "Invalid method"

//---------------------------------Private definitions-------------------------------
static char * currentUsername = NULL;
static char * currentPassword = NULL;

static unsigned handleUsername(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    const char* username = data->pop3Parser.arg;
    if (username == NULL)
        username = "";

    currentUsername = strdup(username);

    writeInBuffer(key, false, NULL, 0);
    selector_set_interest_key(key, OP_WRITE);
    return AUTHORIZATION;
}

static unsigned handlePassword(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    if (currentUsername == NULL) {
        writeInBuffer(key, true, NO_USERNAME, sizeof(NO_USERNAME)-1);
        selector_set_interest_key(key, OP_WRITE);
        return AUTHORIZATION;
    }

    const char* password = data->pop3Parser.arg;
    if(password == NULL)
        password = "";

    currentPassword = strdup(password);

    if(!userLogin(currentUsername, currentPassword)) {
        writeInBuffer(key, true, AUTH_FAILED, sizeof(AUTH_FAILED)-1);
        selector_set_interest_key(key, OP_WRITE);
        currentUsername = NULL;
        currentPassword = NULL;
        return AUTHORIZATION;
    }

    data->isAuth = true;
    writeInBuffer(key, false, AUTH_SUCCESS, sizeof(AUTH_SUCCESS) - 1);
    selector_set_interest_key(key, OP_WRITE);
    return AUTHORIZATION;
}

static unsigned handleQuit(struct selector_key* key) {
    //TODO
}

static unsigned handleUnknown(struct selector_key* key) {
    writeInBuffer(key, true, INVALID_METHOD, sizeof(INVALID_METHOD) - 1);
    selector_set_interest_key(key, OP_WRITE);
    return AUTHORIZATION;
}

//----------------------------------USER Handlers------------------------------------
void authOnArrival(const unsigned state, struct selector_key* key) {
    printf("Entre a authorization\n");
    clientData* data = ATTACHMENT(key);
    resetParser(&data->pop3Parser);
    selector_set_interest_key(key, OP_READ);
}

unsigned authOnReadReady(struct selector_key* key) {
    if (readAndParse(key)) {
        clientData* data = ATTACHMENT(key);
        switch (data->pop3Parser.method) {
            case USER:
                printf("Leo el username: %s\n", data->pop3Parser.arg);
                return handleUsername(key);
            case PASS:
                printf("Leo la contraseña: %s\n", data->pop3Parser.arg);
                return handlePassword(key);
            case QUIT:
                return handleQuit(key);
            default:
                printf("Comando descnocido\n");
                return handleUnknown(key);
        }
    }
    return AUTHORIZATION;
}

unsigned authOnWriteReady(struct selector_key* key) {
    if (!sendFromBuffer(key))
        return AUTHORIZATION;

    clientData* data = ATTACHMENT(key);
    if (data->isAuth)
        return TRANSACTION;

    resetParser(&data->pop3Parser);
    selector_set_interest_key(key, OP_READ);
    printf("Reseteo el parser y vuelvo a authorization\n");
    return AUTHORIZATION;
}



