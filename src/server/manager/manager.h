#ifndef PROTOS_TP_MANAGER_H
#define PROTOS_TP_MANAGER_H

#define HISTORIC_DATA_FILE "historic.csv"

void add_connection(char* username);
void remove_connection();
void add_transferred_bytes(long int bytes);
int store_data_into_file();
int get_stored_data();
void free_print_data();

#endif // PROTOS_TP_MANAGER_H
