#include "users.h"
#include <stdio.h>

#include <stdlib.h>
#include <string.h>

static user users[MAX_USERS];
static int usersCount = 0;
static int adminCount = 0;

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

static void persistUser(const char *username, const char *password, unsigned int role) {
    FILE *file = fopen(USERS_CSV, "a");
    if (!file) {
        perror("Error al abrir el archivo " USERS_CSV "\n");
        return;
    }

    fprintf(file, "%s;%s;%u\n", username, password, role);
    fclose(file);
}

//---------------------------------------Public functions--------------------------------------------
bool userExists(const char* username) {
  int index = getIndexOf(username);
  return index >= 0;
}

int initializeRegisteredUsers() {
    FILE *file = fopen(USERS_CSV, "r");
    if (!file) {
        file = fopen(USERS_CSV, "w");
        if (!file) {
            fprintf(stderr , "Error creating file %s\n", USERS_CSV);
            return -1;
        }
        fclose(file);
        return 0;
    }

    char line[1024];

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        char *username = strtok(line, ";");
        char *password = strtok(NULL, ";");
        char *roleStr = strtok(NULL, "\n");

        if (username && password && roleStr && usersCount < MAX_USERS) {
            strncpy(users[usersCount].username, username, USERS_MAX_USERNAME_LENGTH);
            users[usersCount].username[USERS_MAX_USERNAME_LENGTH] = '\0';
            strncpy(users[usersCount].password, password, USERS_MAX_PASSWORD_LENGTH);
            users[usersCount].password[USERS_MAX_PASSWORD_LENGTH] = '\0';
            if(strcmp(roleStr, "1") == 0){
                users[usersCount].role = ROLE_ADMIN;
                adminCount++;
            } else{
                users[usersCount].role = ROLE_USER;
            }
            usersCount++;
        }
    }

    if(adminCount == 0){
        strncpy(users[usersCount].username, DEFAULT_ADMIN_USERNAME, USERS_MAX_USERNAME_LENGTH);
        strncpy(users[usersCount].password, DEFAULT_ADMIN_PASSWORD, USERS_MAX_PASSWORD_LENGTH);
        users[usersCount].role = ROLE_ADMIN;
        usersCount++;
    }

    fclose(file);
    return 0;
}


void usersCreate(const char* username, const char* password, unsigned int role) {
    if(usersCount >= MAX_USERS) {
        fprintf(stderr, "ERROR: Too many users (%d). Max limit: %d\n", usersCount, MAX_USERS);
        return;
    }
    if(userExists(username) == true) {
        fprintf(stderr, "WARNING: User '%s' already exists. Please try again with another username.\n", username);
        return;
    }

    if(role != ROLE_USER && role != ROLE_ADMIN) {
        fprintf(stderr, "ERROR: Invalid role. Role must be 0 (USER) or 1 (ADMIN).\n");
        return;
    }

    strcpy(users[usersCount].username, username);
    strcpy(users[usersCount].password, password);
    users[usersCount].role = role;
    usersCount++;

    // Agregarlo al csv.
    persistUser(username, password, role);
}

bool userLogin(const char* username, const char* password) {
    int index = getIndexOf(username);
    if(index < 0 || strcmp(users[index].password, password) != 0)
        return false;
    return true;
}
void getUsers(char** user_list) {
    for (int i = 0; i < usersCount; i++) {
        strcpy(user_list[i], users[i].username);
    }
}

int getUsersCount(){
    return usersCount;
}

