#ifndef USERS_H
#define USERS_H

#include <stdbool.h>
#include "../serverConfigs.h"

typedef enum { ROLE_USER = 0, ROLE_ADMIN = 1 } Role;

typedef struct user {
    char *username;
    char *password;
    Role role;
    bool isBlocked;
    struct user * next;
} user;

bool addUser(const char* username, const char* password, unsigned int role);
bool deleteUser(const char* username);
bool blockUser(const char *username, bool block);
bool makeUserAdmin(const char *username);
bool userLogin(const char* username, const char* password);
bool userLoginAdmin(const char* username, const char* password);
bool userExists(const char* username);
void getUsers(char** user_list);
int getUsersCount();

#endif // USERS_H
