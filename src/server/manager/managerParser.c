#include "managerParser.h"

static const methodsMap manager_methods[] = {
    {"USER", USER_M}, {"PASS", PASS_M},
    {"DATA", DATA},{"QUIT", QUIT_M},{"ADDUSER", ADDUSER},
    {"DELUSER", DELUSER},{"SUDO", SUDO},{"BLOCK", BLOCK},
    {"UNBLOCK", UNBLOCK},{"RST", RST},{"SHOW", SHOW},
    {NULL, UNKNOWN_M}
};


const methodsMap* getManagerMethods(){
  return manager_methods;
}