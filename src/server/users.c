#include "users.h"

#include <stdlib.h>
#include <string.h>

#define USERS_MAX_USERNAME_LENGTH 255
#define USERS_MAX_PASSWORD_LENGTH 255
#define MAX_USERS 10

typedef struct {
    char username[USERS_MAX_USERNAME_LENGTH + 1];
    char password[USERS_MAX_PASSWORD_LENGTH + 1];
} user;

static user users[MAX_USERS];
static int usersCount = 0;

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
bool userExists(const char* username) {
  int index = getIndexOf(username);
  return index >= 0;
}

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