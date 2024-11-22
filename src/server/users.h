#ifndef USERS_H
#define USERS_H

#include <stdbool.h>

int initializeRegisteredUsers();
void usersCreate(const char* username, const char* password, unsigned int role);
bool userLogin(const char* username, const char* password);
bool userExists(const char* username);
void getUsers(char** user_list);
int getUsersCount();
#endif //USERS_H
