#ifndef PROTOS_TP_MANAGERPARSER_H
#define PROTOS_TP_MANAGERPARSER_H

#include "../parserUtils.h"

enum manager_methods {
  USER_M,
  PASS_M,
  DATA,
  CAPA_M,
  QUIT_M,
  ADDUSER,
  SUDO,
  BLOCK,
  UNBLOCK,
  RST,
  LOGG,
  ENLOG,
  SETTR,
  ENTR,
  UNKNOWN_M
};

const methodsMap* getManagerMethods();

#endif // PROTOS_TP_MANAGERPARSER_H
