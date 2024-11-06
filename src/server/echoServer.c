#include "echoServer.h"
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>

#define READ_BUFFER_SIZE 2048
#define MAX_HOSTNAME_LENGTH 255

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