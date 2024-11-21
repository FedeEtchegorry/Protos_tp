#include "users.h"
#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#define USERS_CSV "users.csv"

#define USERS_MAX_USERNAME_LENGTH 255
#define USERS_MAX_PASSWORD_LENGTH 255
#define MAX_USERS 10

typedef struct {
    char username[USERS_MAX_USERNAME_LENGTH + 1];
    char password[USERS_MAX_PASSWORD_LENGTH + 1];
    unsigned int isAdmin;
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

static void persistUser(const char *username, const char *password, unsigned int isAdmin) {
    FILE *file = fopen(USERS_CSV, "a");
    if (!file) {
        perror("Error al abrir el archivo users.csv");
        return;
    }

    fprintf(file, "%s;%s;%u\n", username, password, isAdmin);
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
            fprintf(stderr , "Error creating file users.csv");
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
        char *isAdminStr = strtok(NULL, "\n");

        if (username && password && isAdminStr && usersCount < MAX_USERS) {
            strncpy(users[usersCount].username, username, USERS_MAX_USERNAME_LENGTH);
            users[usersCount].username[USERS_MAX_USERNAME_LENGTH] = '\0';
            strncpy(users[usersCount].password, password, USERS_MAX_PASSWORD_LENGTH);
            users[usersCount].password[USERS_MAX_PASSWORD_LENGTH] = '\0';
            users[usersCount].isAdmin = (strcmp(isAdminStr, "1") == 0) ? 1 : 0;
            usersCount++;
        }
    }

    fclose(file);
    return 0;
}


void usersCreate(const char* username, const char* password, unsigned int isAdmin) {
    if(usersCount >= MAX_USERS) {
        fprintf(stderr, "ERROR: Too many users\n");
        return;
    }
    if(userExists(username) == true) {
        fprintf(stderr, "WARNING: User '%s' already exists. Please try again with another username.\n", username);
        return;
    }
    strcpy(users[usersCount].username, username);
    strcpy(users[usersCount].password, password);
    users[usersCount].isAdmin = isAdmin;
    usersCount++;

    // Agregarlo al csv.
    persistUser(username, password, isAdmin);
}

bool userLogin(const char* username, const char* password) {
    int index = getIndexOf(username);
    if(index < 0 || strcmp(users[index].password, password) != 0)
        return false;
    return true;
}