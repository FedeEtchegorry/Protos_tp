#include "echoServer.h"
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "selector.h"

#define READ_BUFFER_SIZE 2048
#define MAX_HOSTNAME_LENGTH 255

//TODO
/* declaración forward de los handlers de selección de una conexión
 * establecida entre un cliente y el proxy.
 */
static void socksv5_read   (struct selector_key *key);
static void socksv5_write  (struct selector_key *key);
static void socksv5_block  (struct selector_key *key);
static void socksv5_close  (struct selector_key *key);
static const struct fd_handler echo_handler = {
  .handle_read   = socksv5_read,
  .handle_write  = socksv5_write,
  .handle_close  = socksv5_close,
  .handle_block  = socksv5_block,
};

//TODO
void pool_destroy(void) {
  struct socks5 *next, *s;
  for(s = pool; s != NULL ; s = next) {
    next = s->next;
    free(s);
  }
}



void passive_accept(struct selector_key * key) {
  struct sockaddr_storage       client_addr;
  socklen_t                     client_addr_len = sizeof(client_addr);
  struct socks5                 *state           = NULL;

  const int client = accept(key->fd, (struct sockaddr*) &client_addr,
                                                        &client_addr_len);
  if(client == -1) {
    goto fail;
  }
  if(selector_fd_set_nio(client) == -1) {
    goto fail;
  }

  //TODO
  state = socks5_new(client);
  if(state == NULL) {
    // sin un estado, nos es imposible manejaro.
    // tal vez deberiamos apagar accept() hasta que detectemos
    // que se liberó alguna conexión.
    goto fail;
  }
  memcpy(&state->client_addr, &client_addr, client_addr_len);
  state->client_addr_len = client_addr_len;

  if(SELECTOR_SUCCESS != selector_register(key->s, client, &socks5_handler,
                                            OP_READ, state)) {
    goto fail;
                                            }
  return ;
  fail:
      if(client != -1) {
        close(client);
      }
  socks5_destroy(state);
}

int handleEchoClient(const int clientFd)
{
  char buffer[READ_BUFFER_SIZE];
  ssize_t bytesRead = recv(clientFd, buffer, READ_BUFFER_SIZE, 0);
  if (bytesRead < 0) {
    perror("recv");
    return -1;
  }

  while(bytesRead > 0) {
    const ssize_t bytesWritten = send(clientFd, buffer, bytesRead, 0);
    if(bytesWritten < 0) {
      perror("send");
      return -1;
    }
    if(bytesWritten != bytesRead) {
      perror("send");
      return -1;
    }
    bytesRead = recv(clientFd, buffer, READ_BUFFER_SIZE, 0);
    if(bytesRead < 0) {
      perror("recv");
      return -1;
    }
  }

  close(clientFd);
  return 0;
}