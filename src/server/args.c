#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <errno.h>
#include <getopt.h>

#include "args.h"

static unsigned short port(const char* s) {
    char* end = 0;
    const long sl = strtol(s, &end, 10);

    if (end == s || '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 0 || sl > USHRT_MAX) {
        fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
        exit(1);
    }

    return sl;
}

static void parseUser(char* s, struct users* user, Role role) {
    char* p = strchr(s, ':');
    if (p == NULL) {
        fprintf(stderr, "password not found\n");
        exit(1);
    }
    *p = 0;
    p++;
    user->name = s;
    user->pass = p;
    user->role = role;
}

static void version(void) {
    fprintf(stderr, "pop3 version 1.0\n"
            "ITBA Protocolos de Comunicación 2024/2 -- Grupo 3\n");
}

/* Estructura de la carpeta para el argumento -d:
$ tree .
.
└── user1
    ├── cur
    ├── new
    │   └── mail1
    └── tmp
*/

static void usage(const char* progname) {
    fprintf(stderr,
            "Usage: %s [OPTION]...\n"
            "\n"
            "   -h               Imprime la ayuda y termina.\n"
            "   -l <POP3 addr>   Dirección donde servirá el servidor POP.\n"
            "   -L <conf  addr>  Dirección donde servirá el servicio de management.\n"
            "   -p <POP3 port>   Puerto entrante conexiones POP3.\n"
            "   -P <conf port>   Puerto entrante conexiones configuracion\n"
            "   -u <name>:<pass> Usuario y contraseña de usuario que puede usar el servidor. Hasta 10.\n"
            "   -U <name>:<pass> Usuario y contraseña del admin que puede usar el servidor. Hasta 10.\n"
            "   -v               Imprime información sobre la versión versión y termina.\n"
            "   -d <dir>         Carpeta donde residen los Maildirs\n"
            "   -t <cmd>         Comando para aplicar transformaciones\n"
            "\n",
            progname);
}

void parseArgs(const int argc, char** argv, struct pop3Args* args) {
    memset(args, 0, sizeof(*args));

    //-------------------------Seteo defaults----------------------------------
    args->socks_addr = "::";
    args->socks_port = 1080;

    args->mng_addr = "::";
    args->mng_port = 8080;

    args->maildir = NULL;
    args->transformation_command = NULL;
    args->transformation_enabled = true;
    args->nusers=0;
    //------------------------Parsear argumentos---------------------------------
    while (true) {
        const int c = getopt(argc, argv, "hl:L:t:p:P:u:U:d:v");
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage(argv[0]);
            exit(1);
        case 'l':
            args->socks_addr = optarg;
            break;
        case 'L':
            args->mng_addr = optarg;
            break;
        case 't':
            args->transformation_command = optarg;
            args->transformation_enabled = true;
            break;
        case 'p':
            args->socks_port = port(optarg);
            break;
        case 'P':
            args->mng_port = port(optarg);
            break;
        case 'd':
            args->maildir = optarg;
            break;
        case 'u':
            if (args->nusers >= MAX_USERS) {
                fprintf(stderr, "maximun number of command line users reached: %d.\n", MAX_USERS);
                exit(1);
            }
            parseUser(optarg, args->users + args->nusers, ROLE_USER);
            args->nusers++;
            break;
        case 'U':
            if (args->nusers >= MAX_USERS) {
                fprintf(stderr, "maximun number of command line users reached: %d.\n", MAX_USERS);
                exit(1);
            }
            parseUser(optarg, args->users + args->nusers, ROLE_ADMIN);
            args->nusers++;
            break;
        case 'v':
            version();
            exit(0);
        default:
            fprintf(stderr, "unknown argument %d.\n", c);
            exit(1);
        }
    }

    if (optind < argc) {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc)
            fprintf(stderr, "%s ", argv[optind++]);
        fprintf(stderr, "\n");
        exit(1);
    }
}
