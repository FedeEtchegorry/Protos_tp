#include "pop3Parser.h"

#include <string.h>

#include "parserUtils.h"

#define BUFFER_SIZE 255


static const methodsMap pop3Methods[] = {
    {"USER", USER}, {"PASS", PASS}, {"LIST", LIST}, {"STAT", STAT},
    {"RSET", RSET}, {"DELE", DELE}, {"NOOP", NOOP},
    {"RETR", RETR}, {"QUIT", QUIT},
    {NULL, UNKNOWN}
};


const methodsMap* getPop3Methods(){
  return pop3Methods;
}