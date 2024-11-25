#include "serverConfigs.h"
#include <stdbool.h>


bool serverBlocked = false;

void setServerBlocked(bool block) {
    serverBlocked = block;
}
bool isServerBlocked() {
    return serverBlocked;
}
