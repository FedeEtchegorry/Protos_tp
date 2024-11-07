#include <stdio.h>
#include <netinet/in.h>

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include "echoServerHandler.h"
#include "echoUtils.h"
#include "utils.h"

#define DEFAULT_PORT 1080

//Falta ver el tema de poder cambiar el ip
int main(int argc, char const* argv[]){
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    int server_fd;
    if (argc != 3) {
        printf("invalid argument count");
        return -1;
    }
    char const *server = argv[1];
    char const * port = argv[2];


    int sock = tcpClientSocket(server, port);
    if (sock < 0) {
        printf("socket creation failed\n");
        return -1;
    }

    printf("Connection established successfully\n");


    handleEchoServer(sock);

    //--------------------------Cierro el client_fd----------------------------------------
    close(sock);
    printf("Connection closed successfully\n");
    return 0;
}
