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

#include "selector.h"

static bool done = false;

static void
sigterm_handler(const int signal) {
  printf("signal %d, cleaning up and exiting\n",signal);
  done = true;
}

int main(const int argc, const char **argv) {
  unsigned port = 1080;

  if(argc == 1) {
    // utilizamos el default
  } else if(argc == 2) {
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

  // no tenemos nada que leer de stdin
  close(0);

  const char       *err_msg = NULL;

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port        = htons(port);

  const int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(server < 0) {
    err_msg = "unable to create socket";
    goto finally;
  }

  fprintf(stdout, "Listening on TCP port %d\n", port);

  // man 7 ip. no importa reportar nada si falla.
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

  if(bind(server, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
    err_msg = "unable to bind socket";
    goto finally;
  }

  if (listen(server, 20) < 0) {
    err_msg = "unable to listen";
    goto finally;
  }

  // registrar sigterm es útil para terminar el programa normalmente.
  // esto ayuda mucho en herramientas como valgrind.
  signal(SIGTERM, sigterm_handler);
  signal(SIGINT,  sigterm_handler);

  finally:
    return 0;
}