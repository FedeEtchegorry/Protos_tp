#ifndef ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8
#define ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8

#include <stdbool.h>

#define MAX_USERS 10

struct users {
    char *name;
    char *pass;
    unsigned int role;
};

struct pop3Args {
    char           *socks_addr;
    unsigned short  socks_port;

    char *          mng_addr;
    unsigned short  mng_port;

    const char *    maildir;

    bool            transformation_enabled;

    struct users    users[MAX_USERS];
    unsigned nusers;
};

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la seleccion humana. Puede cortar
 * la ejecución.
 */
void 
parse_args(const int argc, char **argv, struct pop3Args *args);

#endif

