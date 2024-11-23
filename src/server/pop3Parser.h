#ifndef POP3PARSER_H
#define POP3PARSER_H

#include "buffer.h"
#include "parserUtils.h"

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



#endif //POP3PARSER_H
