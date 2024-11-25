#include "managerParser.h"

static const methodsMap manager_methods[] = {
    {"USER", USER_M}, {"PASS", PASS_M},
    {"DATA", DATA},{"QUIT", QUIT_M},{"ADDUSER", ADDUSER},{NULL, UNKNOWN_M}
};


const methodsMap* getManagerMethods(){
  return manager_methods;
}