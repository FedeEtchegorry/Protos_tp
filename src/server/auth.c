#include "auth.h"

#include <string.h>

#include "POP3Server.h"
#include "buffer.h"

#define VALID_USER "Valid user\n"
#define INVALID_USER "Invalid user\n"
#define VALID_PASS "Valid password\n"
#define INVALID_PASS "Invalid password\n"
#define INVALID_METHOD "Invalid method\n"

static int getIndexOf(const char* username);

//---------------------------------Private definitions-------------------------------
#define USERS_MAX_USERNAME_LENGTH 255
#define USERS_MAX_PASSWORD_LENGTH 255
#define MAX_USERS 10

typedef struct {
    char username[USERS_MAX_USERNAME_LENGTH + 1];
    char password[USERS_MAX_PASSWORD_LENGTH + 1];
} user;

static user users[MAX_USERS];
static int usersCount = 0;

static user currentUser;

//----------------------------------USER Handlers------------------------------------
void userOnArrival(const unsigned state, struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    resetParser(&data->pop3Parser);
    selector_set_interest_key(key, OP_READ);
}

unsigned userOnReadReady(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    if (readAndParse(key)) {
        if(data->pop3Parser.method != USER) {
            writeInBuffer(key, true, INVALID_METHOD, sizeof(INVALID_METHOD) - 1);
            selector_set_interest_key(key, OP_WRITE);
            return AUTH_USER;
        }

        const char* arg = data->pop3Parser.arg;
        if(arg == NULL || getIndexOf(arg) < 0) {
            writeInBuffer(key, true, INVALID_USER, sizeof(INVALID_USER) - 1);
            selector_set_interest_key(key, OP_WRITE);
            return AUTH_USER;
        }

        strcpy(currentUser.username, arg);

        writeInBuffer(key, false, VALID_USER, sizeof(VALID_USER) - 1);
        selector_set_interest_key(key, OP_WRITE);
    }

    return AUTH_USER;
}

unsigned userOnWriteReady(struct selector_key* key) {
    if (!sendFromBuffer(key))
        return AUTH_USER;

    return AUTH_PASS;
}

//--------------------------------------PASS Handlers-----------------------------------------------
void passOnArrival(const unsigned state, struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    resetParser(&data->pop3Parser);
    selector_set_interest_key(key, OP_READ);
}

unsigned passOnReadReady(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);

    if (readAndParse(key)) {
        if(data->pop3Parser.method != PASS) {
            writeInBuffer(key, true, INVALID_METHOD, sizeof(INVALID_METHOD) - 1);
            selector_set_interest_key(key, OP_WRITE);
            return AUTH_USER;
        }

        const char* arg = data->pop3Parser.arg;
        if(arg == NULL)
            return ERROR;

        strcpy(currentUser.password, arg);

        if(!userLogin(currentUser.username, currentUser.password))
            return AUTH_USER;

        writeInBuffer(key, false, VALID_PASS, sizeof(VALID_PASS) - 1);
        selector_set_interest_key(key, OP_WRITE);
    }

    return AUTH_PASS;
}

unsigned passOnWriteReady(struct selector_key* key) {
    if (!sendFromBuffer(key))
        return AUTH_PASS;
    return TRANSACTION;
}

//----------------------------------------Private functions-----------------------------------------
static int getIndexOf(const char* username) {
    int index = -1;
    bool found = false;
    for(int i=0; i < usersCount && !found; i++) {
        if(strcmp(username, users[i].username) == 0) {
            index = i;
            found = true;
        }
    }
    return index;
}


//---------------------------------------Public functions--------------------------------------------
void usersCreate(const char* username, const char* password) {
    if(usersCount >= MAX_USERS)
        return;
    strcpy(users[usersCount].username, username);
    strcpy(users[usersCount].password, password);
    usersCount++;
}

bool userLogin(const char* username, const char* password) {
    int index = getIndexOf(username);
    if(index == -1 || strcmp(users[index].password, password) != 0)
        return false;
    return true;
}

