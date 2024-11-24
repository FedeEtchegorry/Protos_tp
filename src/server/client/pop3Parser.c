#include "pop3Parser.h"
#include <string.h>
#include "../core/buffer.h"

static const methodsMap pop3Methods[] = {
    {"USER", USER}, {"PASS", PASS}, {"LIST", LIST}, {"STAT", STAT},
    {"RSET", RSET}, {"DELE", DELE}, {"NOOP", NOOP},
    {"RETR", RETR}, {"QUIT", QUIT},
    {NULL, UNKNOWN}
};


const methodsMap* getPop3Methods() {
  return pop3Methods;
}