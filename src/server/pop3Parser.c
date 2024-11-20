#include "pop3Parser.h"

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
    buffer[bytesRead - 2] = '\0'; // Eliminar \r\n

    char *command = strtok(buffer, " ");
    char *argument = strtok(NULL, " ");

    if(strcmp(command, "USER") == 0)
        parser->method = USER;
    else if(strcmp(command, "PASS") == 0)
        parser->method = PASS;
    else if(strcmp(command, "LIST") == 0)
        parser->method = LIST;
    else if(strcmp(command, "RETR") == 0)
        parser->method = RETR;
    else
        parser->method = UNKNOWN;

    parser->arg = argument;
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
    parser->state = READING;
    buffer_reset(&parser->buffer);
}



