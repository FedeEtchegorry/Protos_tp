#ifndef USERS_H
#define USERS_H

#include <stdbool.h>
#include "../serverConfigs.h"

typedef enum { ROLE_USER = 0, ROLE_ADMIN = 1 } Role;

typedef struct {
    char username[USERS_MAX_USERNAME_LENGTH + 1];
    char password[USERS_MAX_PASSWORD_LENGTH + 1];
    Role role;
} user;

int initializeRegisteredUsers();
void usersCreate(const char* username, const char* password, unsigned int role);
bool userLogin(const char* username, const char* password);
bool userLoginAdmin(const char* username, const char* password);
bool userExists(const char* username);
void getUsers(char** user_list);
int getUsersCount();

#endif // USERS_H
