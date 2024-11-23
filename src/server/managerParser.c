#include "managerParser.h"

static const methodsMap manager_methods[] = {
    {"USER", USER}, {"PASS", PASS},
    {"DATA", DATA},{"QUIT", QUIT},{NULL, UNKNOWN}
};


const methodsMap* getManagerMethods(){
  return manager_methods;
}