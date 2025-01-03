#include "users.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../serverConfigs.h"
#include "../logging/metrics.h"
#include "../logging/logger.h"

extern server_metrics *clientMetrics;
extern server_logger *logger;

static user * users = NULL;
static int usersCount = 0;
static char infoToLog[256];

//----------------------------------------Private functions-----------------------------------------
static user * getUserByUsername(const char* username) {

    if (username == NULL) {
        return NULL;
    }
    user * current = users;
    while(current) {
        if(strcmp(username, current->username) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}
//---------------------------------------Public functions--------------------------------------------

bool userExists(const char* username) {
  return getUserByUsername(username) != NULL;
}

bool addUser(const char* username, const char* password, const unsigned int role) {
    unsigned int usernameLength = strlen(username);
    unsigned int passwordLength = strlen(password);

    if(usersCount >= MAX_USERS) {
        fprintf(stderr, "ERROR: Too many users (%d). Max limit: %d\n", usersCount, MAX_USERS);
        return false;
    }
    if(userExists(username) == true) {
        fprintf(stderr, "WARNING: User '%s' already exists. Please try again with another username.\n", username);
        return false;
    }

    if(role != ROLE_USER && role != ROLE_ADMIN) {
        fprintf(stderr, "ERROR: Invalid role. Role must be 0 (USER) or 1 (ADMIN).\n");
        return false;
    }

    if(usernameLength > USERS_MAX_USERNAME_LENGTH) {
        fprintf(stderr, "ERROR: Username too long. Max length: %d\n", USERS_MAX_USERNAME_LENGTH);
        return false;
    }

    if(passwordLength > USERS_MAX_PASSWORD_LENGTH) {
        fprintf(stderr, "ERROR: Password too long. Max length: %d\n", USERS_MAX_PASSWORD_LENGTH);
        return false;

    }

    user * newUser = malloc(sizeof(user));
    if(newUser == NULL) {
        fprintf(stderr, "ERROR: Insuficient memory to allocate a new user\n");
        return false;
    }
    if(users == NULL) {
        users = newUser;
        newUser->next = NULL;
    } else {
        newUser->next = users;
        users = newUser;
    }

    newUser->username = malloc(usernameLength + 1);
    newUser->password = malloc(passwordLength + 1);

    strcpy(newUser->username, username);
    strcpy(newUser->password, password);

    newUser->role = role;
    usersCount++;

    snprintf(infoToLog, sizeof(infoToLog), "New user '%s' has been created", username);
    serverLoggerRegister(logger, infoToLog);

    serverMetricsRecordNewUser(clientMetrics);

    return true;
}

bool makeUserAdmin(const char *username) {
    user * maybeUser = getUserByUsername(username);

    if(maybeUser != NULL)
    {
        maybeUser->role = ROLE_ADMIN;
        snprintf(infoToLog, sizeof(infoToLog), "User '%s' has been promoted to Admin", username);
        serverLoggerRegister(logger, infoToLog);

        return true;
    }
    return false;
}


bool userLogin(const char* username, const char* password) {
    user * maybeLoggedUser = getUserByUsername(username);
    if (maybeLoggedUser == NULL || strcmp(maybeLoggedUser->password, password) != 0 || isServerBlocked())
        return false;

    snprintf(infoToLog, sizeof(infoToLog), "User '%s' has logged", username);
    serverLoggerRegister(logger, infoToLog);

    return true;
}

bool userLoginAdmin(const char* username, const char* password){
    user * maybeLoggedAdmin = getUserByUsername(username);
    if(maybeLoggedAdmin == NULL || strcmp(maybeLoggedAdmin->password, password) != 0 || maybeLoggedAdmin->role != ROLE_ADMIN) {
        return false;
    }
    return true;
}

int getUsersCount(){
    return usersCount;
}

