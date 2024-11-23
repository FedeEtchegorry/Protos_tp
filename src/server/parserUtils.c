#include "parserUtils.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

static int getMethod(const char *command, parser* parser) {
  for (const methodsMap *entry = parser->all_methods; entry->command != NULL; entry++) {
    if (strcmp(entry->command, command) == 0) {
      return entry->method;
    }
  }
  return parser->unknown_method;
}


void parserInit(parser * parser, methodsMap* methods) {
  uint8_t * auxBuffer = malloc(BUFFER_SIZE);
  buffer_init(&parser->buffer, BUFFER_SIZE, auxBuffer);
  parser->state = READING;
  parser->all_methods = methods;
  int i=0;
  while (methods[i].command!=NULL) {
    i++;
  }
  parser->unknown_method=i;
}


void processBuffer(parser *parser) {
  char buffer[BUFFER_SIZE + 1] = {0};

  size_t bytesRead;
  uint8_t *readBuffer = buffer_read_ptr(&parser->buffer, &bytesRead);

  if (bytesRead == 0 || bytesRead > BUFFER_SIZE) {
    fprintf(stderr, "Invalid buffer size\n");
    return;
  }

  memcpy(buffer, readBuffer, bytesRead);

  int inc = parser->isCRLF ? 2 : 1;
  if (bytesRead >= inc) {
    buffer[bytesRead - inc] = '\0';
  }

  char *command = strtok(buffer, " ");
  if (command == NULL) {
    fprintf(stderr, "No command found\n");
    return;
  }

  for (char *p = command; *p != '\0'; ++p) {
    *p = toupper((unsigned char)*p);
  }

  parser->method = getMethod(command, parser);

  char *argument = strtok(NULL, " ");
  if (argument != NULL) {
    parser->arg = strdup(argument);
    if (parser->arg == NULL) {
      fprintf(stderr, "Memory allocation failed for argument\n");
    }
  }

  printf("Command: %s, Method: %d, Arg: %s\n",
         command, parser->method, parser->arg ? parser->arg : "(none)");
}


void parse_feed(parser * parser, uint8_t c) {
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

void parse(parser * parser, buffer * buffer) {
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

bool parserIsFinished(parser * parser) {
  return parser->state == READY;
}

void resetParser(parser * parser) {
  buffer_reset(&parser->buffer);
  if (parser->arg != NULL)
    free(parser->arg);
  parser->arg = NULL;
  parser->isCRLF = false;
  parser->method = parser->unknown_method;
  parser->state = READING;
}

void parserDestroy(parser * parser) {
  free(parser->arg);
}


