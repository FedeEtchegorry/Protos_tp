#include "echoServerHandler.h"
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>

#define READ_BUFFER_SIZE 2048
#define WRITE_BUFFER_SIZE 2048
#define MAX_HOSTNAME_LENGTH 255

int handleEchoServer(const int serverFd) {
    char bufferToWrite[READ_BUFFER_SIZE]={0};
    char bufferToRead[WRITE_BUFFER_SIZE]={0};
    ssize_t size_input;
    while ((size_input = read(STDIN_FILENO, bufferToWrite, sizeof(bufferToWrite))) != 0) {
        const ssize_t bytesWritten = send(serverFd, bufferToWrite, size_input, 0);
        if (bytesWritten < 0 || bytesWritten != size_input) {
            printf("send");
            close(serverFd);
            return -1;
        }
        ssize_t bytesRead = recv(serverFd, bufferToRead, READ_BUFFER_SIZE, 0);
        printf("%s", bufferToRead);
        for (int i=0;i<bytesRead;i++){
            bufferToRead[i]=0;
        }
    }
    close(serverFd);
    return 0;
}