#ifndef POP3PARSER_H
#define POP3PARSER_H

#include "buffer.h"

enum methods {
  USER,
  PASS,
  STAT,
  LIST,
  RETR,
  DELE,
  RSET,
  NOOP,
  QUIT,
  UNKNOWN
};

enum state {
  READING,
  READY,
};

typedef struct pop3Parser {
  enum state state;

  enum methods method;
  char * arg;

  buffer buffer;
} pop3Parser;

void parserInit(pop3Parser * parser);
void resetParser(pop3Parser * parser);
bool parserIsFinished(pop3Parser * parser);
void parse(pop3Parser * parser, buffer * buffer);

#endif //AUTH_H
