#ifndef PROTOS_TP_MANAGERPARSER_H
#define PROTOS_TP_MANAGERPARSER_H

#include "../parserUtils.h"

enum manager_methods {
  USER_M,
  PASS_M,
  ADDUSER,
  DELUSER,
  BLOCK,
  UNBLOCK,
  SUDO,
  RST,
  SHOW,
  DATA,
  QUIT_M,
  UNKNOWN_M
};

const methodsMap* getManagerMethods();

#endif // PROTOS_TP_MANAGERPARSER_H
