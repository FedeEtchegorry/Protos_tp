#include <stdio.h>
#include <netinet/in.h>

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include "echoServerHandler.h"

#define DEFAULT_PORT 1080

//Falta ver el tema de poder cambiar el ip
int main(int argc, char const* argv[]){
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    int server_fd;
    //--------------------------Setear numero de puerto en base a stdin-----------------
    unsigned port;
    if(argc == 1)
        port = DEFAULT_PORT;
    else if(argc == 2) {
        char * end     = 0;
        const long sl = strtol(argv[1], &end, 10);

        if (end == argv[1]|| '\0' != *end
            || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
            || sl < 0 || sl > USHRT_MAX) {
            fprintf(stderr, "port should be an integer: %s\n", argv[1]);
            return 1;
        }
        port = sl;
    } else {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    //-------------------------Abro socket y consigo el fd---------------------------------
    if ((server_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("socket");
        return -1;
    }
    //--------------------------Defino estructura para el socket para soportar IPv6--------
    struct sockaddr_in6 addr;
    addr.sin6_family      = AF_INET6;
    addr.sin6_port        = htons(port);
    addr.sin6_addr        = in6addr_any;

    if (connect(server_fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        printf("Connection Failed, you should check if the server is working \n");
        return -1;
    }
    printf("Connection established successfully\n");


    handleEchoServer(server_fd);

    //--------------------------Cierro el client_fd----------------------------------------
    close(server_fd);
    printf("Connection closed successfully\n");
    return 0;
}
