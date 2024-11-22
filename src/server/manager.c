#include "manager.h"
#include "users.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#define HISTORIC_DATA_FILE "historic.csv"
#define USERS_MAX_USERNAME_LENGTH 255 //COMBINAR CON EL DE USERS.C

struct manager_data{
    long int total_connections;
    long int connected_now;
    long int total_transferred_bytes;
    long int total_users;
    char** user_list;
};

static struct manager_data manager={
    .total_connections=0,
    .connected_now=0,
    .total_transferred_bytes=0,
    .total_users=0,
    .user_list=NULL,

};

//----------------------------------------Private functions-----------------------------------------
static void sum_to_connections(char* username){
    manager.total_connections+=1;
    manager.connected_now+=1;
    if (!userExists(username)) {
        manager.total_users += 1;
    }
}
static void sum_multiple_connections(long int connections, long int users){
    manager.total_connections+=connections;
    manager.total_users+=users;
}
static void remove_connected_now(){
    manager.connected_now-=1;
}
static void sum_to_transferred_bytes(long int bytes){
    manager.total_transferred_bytes+=bytes;
}
//----------------------------------------Public functions-----------------------------------------
void add_connection(char* username){
    sum_to_connections(username);
}
void remove_connection(){
    remove_connected_now();
}
void add_transferred_bytes(long int bytes){
    sum_to_transferred_bytes(bytes);
}
const struct manager_data* print_data() {
    if (!manager.user_list)
        free_print_data();
    manager.user_list = malloc(getUsersCount() * sizeof(char *));
    if (!manager.user_list) {
        fprintf(stderr, "Error allocating memory for user list.\n");
        return NULL;
    }

    for (int i = 0; i < getUsersCount(); i++) {
       manager.user_list[i] = malloc(USERS_MAX_USERNAME_LENGTH + 1);
    }
    getUsers(manager.user_list);
    return &manager;
}
void free_print_data(){
    for (int i = 0; i < manager.total_users; i++) {
        free(manager.user_list[i]);
    }
    free(manager.user_list);
}



/*
 * total_connections; total_transferred_bytes; total_users\n
 */
int get_stored_data(){
    FILE *file = fopen(HISTORIC_DATA_FILE, "r");
    if (!file) {
        file = fopen(HISTORIC_DATA_FILE, "w");
        if (!file) {
          fprintf(stderr , "Error creating file users.csv");
          return -1;
        }
        fclose(file);
        return 0;
    }
    char line[30];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '\r') {
          continue;
        }
        long int connections=strtol(strtok(line, ";"), NULL, 10);
        long int users=strtol(strtok(NULL, "\n"), NULL, 10);
        long int bytes=strtol(strtok(NULL, ";"), NULL, 10);


        sum_multiple_connections(connections, users);
        sum_to_transferred_bytes(bytes);
    }
    fclose(file);
    return 0;
}

int store_data_into_file(){
    FILE *file = fopen(HISTORIC_DATA_FILE, "w+");
    if (!file) {
        fprintf(stderr,"Error al abrir el archivo users.csv");
        return 1;
    }
    fprintf(file, "%lu;%lu;%lu\n", manager.total_connections, manager.total_transferred_bytes, manager.total_users);
    fclose(file);
    return 0;
}
