#include "managerParser.h"

static const methodsMap manager_methods[] = {
    {"USER", USER_M}, {"PASS", PASS_M},
    {"DATA", DATA},{"CAPA", CAPA_M},{"QUIT", QUIT_M},{NULL, UNKNOWN_M}
};


const methodsMap* getManagerMethods(){
  return manager_methods;
}