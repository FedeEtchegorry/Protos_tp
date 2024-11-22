#include "pop3Parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "POP3Server.h"
#include "selector.h"

#define BUFFER_SIZE 255

void parserInit(pop3Parser * parser) {
    uint8_t * auxBuffer = malloc(BUFFER_SIZE);
    buffer_init(&parser->buffer, BUFFER_SIZE, auxBuffer);
    parser->state = READING;
}

static void processBuffer(pop3Parser * parser) {
    char buffer[BUFFER_SIZE + 1];

    size_t bytesRead;
    uint8_t * readBuffer = buffer_read_ptr(&parser->buffer, &bytesRead);

    memcpy(buffer, readBuffer, bytesRead);
    int inc;
    if (parser->isCRLF)
        inc = 2;
    else
        inc = 1;
    buffer[bytesRead - inc] = '\0';

    char *command = strtok(buffer, " ");
    for (unsigned long i = 0; i < strlen(command); i++)
        command[i] = toupper(command[i]);
    char *argument = strtok(NULL, " ");

    if(strcmp(command, "USER") == 0)
        parser->method = USER;
    else if(strcmp(command, "PASS") == 0)
        parser->method = PASS;
    else if(strcmp(command, "LIST") == 0)
        parser->method = LIST;
    else if (strcmp(command, "STAT") == 0)
        parser->method = STAT;
    else if (strcmp(command, "RSET") == 0)
        parser->method = RSET;
    else if (strcmp(command, "DELE") == 0)
        parser->method = DELE;
    else if (strcmp(command, "NOOP") == 0)
        parser->method = NOOP;
    else if (strcmp(command, "DATA") == 0)
        parser->method = DATA;
    else if(strcmp(command, "RETR") == 0)
        parser->method = RETR;
    else if (strcmp(command, "QUIT") == 0)
        parser->method = QUIT;
    else
        parser->method = UNKNOWN;


    if (argument != NULL)
        parser->arg = strdup(argument);

    printf("Command: %s\n", command);
}

void parse_feed(pop3Parser * parser, uint8_t c) {
    buffer_write(&parser->buffer, c);
    if (c == '\r')
        parser->isCRLF = true;
    else if (c != '\n')
        parser->isCRLF = false;

    if (c == '\n') {
        processBuffer(parser);
        parser->state = READY;
    }
}

void parse(pop3Parser * parser, buffer * buffer) {
    size_t readable;
    const uint8_t * readBuffer = buffer_read_ptr(buffer, &readable);

    size_t writeLimit;
    uint8_t * writeBuffer = buffer_write_ptr(&parser->buffer, &writeLimit);

    if (readable > writeLimit) {
        return;
    }

    memcpy(writeBuffer, readBuffer, readable);

    buffer_read_adv(buffer, readable);
    buffer_write_adv(&parser->buffer, readable);

    size_t auxReadable;
    const uint8_t * auxReadBuffer = buffer_read_ptr(&parser->buffer, &auxReadable);
    if(auxReadable > 2 && auxReadBuffer[auxReadable - 1] == '\n' && auxReadBuffer[auxReadable - 2] == '\r') {
        processBuffer(parser);
        parser->state = READY;
    }
}

bool parserIsFinished(pop3Parser * parser) {
    return parser->state == READY;
}

void resetParser(pop3Parser * parser) {
    buffer_reset(&parser->buffer);
    if (parser->arg != NULL)
        free(parser->arg);
    parser->arg = NULL;
    parser->isCRLF = false;
    parser->method = UNKNOWN;
    parser->state = READING;
}

void parserDestroy(pop3Parser * parser) {
	free(parser->arg);
}


