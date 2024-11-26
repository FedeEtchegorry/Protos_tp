#include "serverConfigs.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>


bool serverBlocked = false;
bool transformationEnabled = false;
char transformationCommand[MAX_SIZE_TRANSFORMATION_CMD] = {0};

void setServerBlocked(bool block) {
    serverBlocked = block;
}
bool isServerBlocked() {
    return serverBlocked;
}


bool setTransformationEnabled(bool enabled) {
  	if(enabled == true && transformationCommand[0] == '\0'){
         setTransformationEnabled(false);
    	return false;
    }
    transformationEnabled = enabled;
    return true;
}

bool isTransformationEnabled() {
    return transformationEnabled;
}

int setTransformationCommand(const char* command) {
    if (command == NULL) {
        setTransformationEnabled(false);
        return ERR_NULL_COMMAND;
    }
    if (strlen(command) > MAX_SIZE_TRANSFORMATION_CMD - 1) {
        setTransformationEnabled(false);
        return ERR_COMMAND_TOO_LONG;
    }
    if (command[0] == '\0') {
        setTransformationEnabled(false);
        return ERR_EMPTY_COMMAND;
    }
    if (access(command, F_OK) != 0 || access(command, X_OK) != 0) {
        setTransformationEnabled(false);
        return ERR_INVALID_PATH;
    }

    strcpy(transformationCommand, command);
    setTransformationEnabled(true);
    return SUCCESS;
}

char * getTransformationCommand(){
  return transformationCommand;
}