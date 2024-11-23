#ifndef PROTOS_TP_PARSERUTILS_H
#define PROTOS_TP_PARSERUTILS_H

#include "buffer.h"


enum state {
  READING,
  READY,
};

typedef struct {
  const char *command;
  int method;
} methodsMap;


typedef struct parser {
  char *arg;
  bool isCRLF;
  int method;
  int state;
  buffer buffer;
  const methodsMap *all_methods;
  int unknown_method;
} parser;

void processBuffer(parser *parser);
void parserInit(parser * parser, const methodsMap* methods);
void parse_feed(parser * parser, uint8_t c);
void parse(parser * parser, buffer * buffer);
void resetParser(parser * parser);
bool parserIsFinished(parser * parser);



#endif // PROTOS_TP_PARSERUTILS_H
