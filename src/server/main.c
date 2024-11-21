#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>  // socket
#include <netinet/in.h>

#include "POP3Server.h"
#include "selector.h"
#include "args.h"
#include "users.h"

#define DEFAULT_PORT 1080
#define MAX_PENDING_CONNECTION_REQUESTS 5
#define SELECTOR_SIZE 1024

static bool done = false;

static void sigterm_handler(const int signal) {
    printf("signal %d, cleaning up and exiting\n", signal);
    done = true;
}

int main(const int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    close(STDIN_FILENO);

    //-------------------Levantar usuarios ya registrados del archivos users.csv ---------------------




    //--------------------------Parsear argumentos por stdin-----------------
    struct pop3Args args;
    parse_args(argc, argv, &args);

    //Defined here to be used in finally
    char* err_msg = NULL;
    selector_status ss = SELECTOR_SUCCESS;
    int server = -1;
    fd_selector selector = NULL;

    for(int i = 0; i < args.nusers; i++)
        usersCreate(args.users[i].name, args.users[i].pass, args.users[i].isAdmin);

    //--------------------------Defino estructura para el socket para soportar IPv6--------
    struct sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(args.socks_port);
    if (inet_pton(AF_INET6, args.socks_addr, &addr.sin6_addr) != 1) {
        err_msg = "Invalid IPv6 address";
        goto finally;
    }

    //-------------------------Abro socket y consigo el fd---------------------------------
    server = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (server < 0) {
        err_msg = "unable to create socket";
        goto finally;
    }

    fprintf(stdout, "Listening on TCP port %d\n", args.socks_port);

    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    //-------------------------Bindeo el socket con la estructura creada-------------------
    if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        err_msg = "unable to bind socket";
        goto finally;
    }

    if (listen(server, MAX_PENDING_CONNECTION_REQUESTS) < 0) {
        err_msg = "unable to listen";
        goto finally;
    }

    //-------------------------Registro el sigHandler para la terminacion del programa------
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    //-------------------------Inicializo el selector---------------------------------------
    if (selector_fd_set_nio(server) == -1) {
        err_msg = "getting server socket flags";
        goto finally;
    }

    const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec = 10,
            .tv_nsec = 0,
        },
    };

    if (selector_init(&conf) != 0) {
        err_msg = "initializing selector";
        goto finally;
    }

    selector = selector_new(SELECTOR_SIZE);
    if (selector == NULL) {
        err_msg = "unable to create selector";
        goto finally;
    }

    //-----------------------------Registro a mi socket pasivo para que acepte conexiones---------------------
    const fd_handler passive_socket = {
        .handle_read = pop3_passive_accept,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    ss = selector_register(selector, server, &passive_socket, OP_READ, NULL);
    if (ss != SELECTOR_SUCCESS) {
        err_msg = "registering fd";
        goto finally;
    }

    //----------------------------Itero infinitamente haciendo select------------------------------------------
    while (!done) {
        ss = selector_select(selector);
        if (ss != SELECTOR_SUCCESS) {
            err_msg = "serving";
            goto finally;
        }
    }

    err_msg = "Closing";

    int ret = 0;
finally:
    if (ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", (err_msg == NULL) ? "" : err_msg,
                ss == SELECTOR_IO
                    ? strerror(errno)
                    : selector_error(ss));
        ret = 2;
    } else {
        perror(err_msg);
        ret = 1;
    }

    if(selector != NULL)
        selector_destroy(selector);
    selector_close();
    if(server >= 0)
        close(server);

    return ret;
}
