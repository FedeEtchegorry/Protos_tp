#ifndef USERS_H
#define USERS_H

#include <stdbool.h>

int initializeRegisteredUsers();
void usersCreate(const char* username, const char* password, unsigned int isAdmin);
bool userLogin(const char* username, const char* password);
bool userExists(const char* username);

#endif //USERS_H
