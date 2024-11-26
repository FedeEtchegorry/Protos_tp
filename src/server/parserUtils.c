#include "parserUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "client/pop3Parser.h"

#define BUFFER_SIZE 1024

enum state {
  READING,
  READY,
};

typedef struct parserCDT {
  char *arg1;
  char *arg2;
  buffer buffer;
  const methodsMap *all_methods;
  int unknown_method;
  int method;
  int state;
  bool isCRLF;
} parserCDT;

static int getMethod(const char *command, parserADT parser) {
  for (const methodsMap *entry = parser->all_methods; entry->command != NULL; entry++) {
    if (strcmp(entry->command, command) == 0) {
      return entry->method;
    }
  }
  return parser->unknown_method;
}


parserADT parserInit(const methodsMap * methods) {
  parserADT parser = malloc(sizeof(parserCDT));
  uint8_t * auxBuffer = malloc(BUFFER_SIZE);
  buffer_init(&parser->buffer, BUFFER_SIZE, auxBuffer);

  parser->state = READING;
  parser->all_methods = methods;

  int i=0;
  while (methods[i].command!=NULL)
    i++;
  parser->unknown_method=i;
  parser->method=i;
  parser->isCRLF=false;
  parser->arg1 = NULL;
  parser->arg2 = NULL;
  return parser;
}

void parserDestroy(parserADT parser) {
  if (parser->arg1 != NULL)
    free(parser->arg1);
  if (parser->arg2 != NULL)
    free(parser->arg2);
  free(parser->buffer.data);
  free(parser);
}


static void processBuffer(parserADT parser) {
  char buffer[BUFFER_SIZE + 1] = {0};

  size_t bytesRead;
  uint8_t *readBuffer = buffer_read_ptr(&parser->buffer, &bytesRead);

  if (bytesRead == 0 || bytesRead > BUFFER_SIZE) {
    fprintf(stderr, "Invalid buffer size\n");
    return;
  }

  memcpy(buffer, readBuffer, bytesRead);

  int extra = parser->isCRLF ? 2 : 1;
  buffer[bytesRead - extra] = '\0';

  char *command = strtok(buffer, " ");
  if (command == NULL) {
    fprintf(stderr, "No command found\n");
    return;
  }

  for (char *p = command; *p != '\0'; ++p)
    *p = toupper((unsigned char)*p);

  parser->method = getMethod(command, parser);

  char *argument = strtok(NULL, " ");
  if (argument != NULL) {
    parser->arg1 = strdup(argument);
    if (parser->arg1 == NULL) {
      fprintf(stderr, "Memory allocation failed for argument\n");
    }
  }

  char *argument2 = strtok(NULL, "\n");
  if (argument2 != NULL) {
    parser->arg2 = strdup(argument2);
    if (parser->arg2 == NULL) {
      fprintf(stderr, "Memory allocation failed for argument\n");
    }
  }

  //printf("Command: %s, Method: %d, Arg: %s, Arg2: %s\n", command, parser->method, parser->arg1 ? parser->arg1 : "(none)", parser->arg2 ? parser->arg2 : "(none)");
}

void parse_feed(parserADT parser, uint8_t c) {
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

char * parserGetFirstArg(parserADT parser) {
  return parser->arg1;
}
char * parserGetExtraArg(parserADT parser) {
  return parser->arg2;
}
unsigned parserGetMethod(parserADT parser) {
  return parser->method;
}

bool parserIsFinished(parserADT parser) {
  return parser->state == READY;
}

void resetParser(parserADT parser) {
  buffer_reset(&parser->buffer);
  if (parser->arg1 != NULL)
    free(parser->arg1);
  if (parser->arg2 != NULL)
    free(parser->arg2);
  parser->arg1 = NULL;
  parser->arg2 = NULL;
  parser->isCRLF = false;
  parser->method = parser->unknown_method;
  parser->state = READING;
}




