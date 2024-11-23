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
  DATA,
  QUIT,
  UNKNOWN
};

enum state {
  READING,
  READY,
};

typedef struct pop3Parser {
  enum state state;

  bool isCRLF;

  enum methods method;
  char * arg;

  buffer buffer;
} pop3Parser;

void parserInit(pop3Parser * parser);
bool parserIsFinished(pop3Parser * parser);
void resetParser(pop3Parser * parser);
void parse(pop3Parser * parser, buffer * buffer);
void parse_feed(pop3Parser * parser, uint8_t c);

#endif //POP3PARSER_H
