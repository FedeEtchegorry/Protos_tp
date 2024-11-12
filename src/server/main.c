#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>   // socket
#include <sys/socket.h>  // socket
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "echoServer.h"

#include "selector.h"

#define DEFAULT_PORT 1080
#define MAX_PENDING_CONNECTION_REQUESTS 5
#define SELECTOR_SIZE 1024

static bool done = false;

static void sigterm_handler(const int signal)
{
    printf("signal %d, cleaning up and exiting\n", signal);
    done = true;
}


int main(const int argc, const char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    //--------------------------Setear numero de puerto en base a stdin-----------------
    unsigned port;
    if (argc == 1)
        port = DEFAULT_PORT;
    else if (argc == 2)
    {
        char* end = 0;
        const long sl = strtol(argv[1], &end, 10);

        if (end == argv[1] || '\0' != *end
            || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
            || sl < 0 || sl > USHRT_MAX)
        {
            fprintf(stderr, "port should be an integer: %s\n", argv[1]);
            return 1;
        }
        port = sl;
    }
    else
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    //--------------------------Cerrar stdin porque no hay nada para leer------------------
    close(0);


    //----------------------------Variables de programa------------------------------------
    const char* err_msg = NULL;
    selector_status ss = SELECTOR_SUCCESS;
    fd_selector selector = NULL;

    //--------------------------Defino estructura para el socket para soportar IPv6--------
    struct sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    addr.sin6_addr = in6addr_any;

    //-------------------------Abro socket y consigo el fd---------------------------------
    const int server = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (server < 0)
    {
        err_msg = "unable to create socket";
        goto finally;
    }

    fprintf(stdout, "Listening on TCP port %d\n", port);

    //TODO PREGUNTAR PORQUE SE USAN ESTAS FLAGS
    // man 7 ip. no importa reportar nada si falla.
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    //-------------------------Bindeo el socket con la estructura creada-------------------
    if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        err_msg = "unable to bind socket";
        goto finally;
    }

    if (listen(server, MAX_PENDING_CONNECTION_REQUESTS) < 0)
    {
        err_msg = "unable to listen";
        goto finally;
    }

    //-------------------------Registro el sigHandler para la terminacion del programa------
    // esto ayuda mucho en herramientas como valgrind.
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    //-------------------------Inicializo el selector---------------------------------------
    if (selector_fd_set_nio(server) == -1)
    {
        err_msg = "getting server socket flags";
        goto finally;
    }


    //TODO preguntar para se usa el timeout
    const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec = 10,
            .tv_nsec = 0,
        },
    };

    if (0 != selector_init(&conf))
    {
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
        .handle_read = passive_accept,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    ss = selector_register(selector, server, &passive_socket,OP_READ, NULL);
    if (ss != SELECTOR_SUCCESS) {
        err_msg = "registering fd";
        goto finally;
    }

    //----------------------------Itero infinitamente haciendo select------------------------------------------
    while(!done) {
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            err_msg = "serving";
            goto finally;
        }
    }

    if(err_msg == NULL)
        err_msg = "closing";


    /*
    while (!done) {
        printf("Listening for next client...\n");
        struct sockaddr_storage clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);
        const int clientHandleSocket = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLen);
        if (clientHandleSocket < 0)
        {
            err_msg = "unable to accept client";
            goto finally;
        }
        printf("Client connected\n");

        handleEchoClient(clientHandleSocket);

        printf("Client disconnected\n");
    }
    */
finally:
    int ret = 0;

    if(ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", (err_msg == NULL) ? "": err_msg,
                                  ss == SELECTOR_IO
                                      ? strerror(errno)
                                      : selector_error(ss));
        ret = 2;
    } else if(err_msg) {
        perror(err_msg);
        ret = 1;
    }
    if(selector != NULL) {
        selector_destroy(selector);
    }
    selector_close();

    pool_destroy();

    if(server >= 0)
        close(server);
    return ret;
}
