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

static bool done = false;

static void
sigterm_handler(const int signal) {
  printf("signal %d, cleaning up and exiting\n",signal);
  done = true;
}

int main(const int argc, const char **argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

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

  //--------------------------Cerrar stdin porque no hay nada para leer------------------
  close(0);


  const char * err_msg = NULL;

  //--------------------------Defino estructura para el socket para soportar IPv6--------
  struct sockaddr_in6 addr = {0};
  addr.sin6_family      = AF_INET6;
  addr.sin6_port        = htons(port);
  addr.sin6_addr = in6addr_any;

  //-------------------------Abro socket y consigo el fd---------------------------------
  const int server = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  if(server < 0) {
    err_msg = "unable to create socket";
    goto finally;
  }

  fprintf(stdout, "Listening on TCP port %d\n", port);

  // man 7 ip. no importa reportar nada si falla.
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

  //-------------------------Bindeo el socket con la estructura creada-------------------
  if(bind(server, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
    err_msg = "unable to bind socket";
    goto finally;
  }

  if (listen(server, MAX_PENDING_CONNECTION_REQUESTS) < 0) {
    err_msg = "unable to listen";
    goto finally;
  }

  // registrar sigterm es útil para terminar el programa normalmente.
  // esto ayuda mucho en herramientas como valgrind.
  signal(SIGTERM, sigterm_handler);
  signal(SIGINT,  sigterm_handler);

  while (!done) {
    printf("Listening for next client...\n");
    struct sockaddr_storage clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);
    const int clientHandleSocket = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLen);
    if (clientHandleSocket < 0) {
      err_msg = "unable to accept client";
      goto finally;
    }
    printf("Client connected\n");

    handleEchoClient(clientHandleSocket);

    printf("Client disconnected\n");
  }

  finally:
    close(server);
    if(err_msg!=NULL){
      fprintf(stderr, "%s\n", err_msg);
      return -1;
    }
    return 0;
}