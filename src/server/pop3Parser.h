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

typedef struct pop3Parser pop3Parser;

enum methods parserGetMethod(pop3Parser *parser);
char * parserGetArg(pop3Parser *parser);
pop3Parser * parserInit();
bool parserIsFinished(pop3Parser * parser);
void resetParser(pop3Parser * parser);
void parse(pop3Parser * parser, buffer * buffer);
void parse_feed(pop3Parser * parser, uint8_t c);
void parserDestroy(pop3Parser * parser);

#endif //AUTH_H
