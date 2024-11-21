#ifndef USERS_H
#define USERS_H

#include <stdbool.h>

void usersCreate(const char* username, const char* password);
bool userLogin(const char* username, const char* password);
bool userExists(const char* username);

#endif //USERS_H
