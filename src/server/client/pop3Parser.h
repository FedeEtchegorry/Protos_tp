#ifndef POP3PARSER_H
#define POP3PARSER_H

#include "../parserUtils.h"

enum methods {
  USER,
  PASS,
  STAT,
  LIST,
  RETR,
  DELE,
  RSET,
  NOOP,
  CAPA,
  UIDL,
  QUIT,
  UNKNOWN
};

const methodsMap* getPop3Methods();

#endif // POP3PARSER_H
