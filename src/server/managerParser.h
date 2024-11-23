#ifndef PROTOS_TP_MANAGERPARSER_H
#define PROTOS_TP_MANAGERPARSER_H

#include "parserUtils.h"

enum methods {
  USER,
  PASS,
  DATA,
  QUIT,
  UNKNOWN
};
const methodsMap* getManagerMethods();

#endif // PROTOS_TP_MANAGERPARSER_H
