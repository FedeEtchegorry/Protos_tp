#ifndef PROTOS_TP_PARSERUTILS_H
#define PROTOS_TP_PARSERUTILS_H

#include "./core/buffer.h"

typedef struct {
  const char *command;
  int method;
} methodsMap;

typedef struct parserCDT * parserADT;

parserADT parserInit(const methodsMap* methods);
void parserDestroy(parserADT parser);

void parse_feed(parserADT parser, uint8_t c);
char * parserGetFirstArg(parserADT parser);
char * parserGetExtraArg(parserADT parser);
unsigned parserGetMethod(parserADT parser);
bool parserIsFinished(parserADT parser);
void resetParser(parserADT parser);

#endif // PROTOS_TP_PARSERUTILS_H
