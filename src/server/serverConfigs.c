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

void setTransformationCommand(const char* command){
  	if(command == NULL || strlen(command) > MAX_SIZE_TRANSFORMATION_CMD || command[0] == '\0' || strlen(command) > MAX_SIZE_TRANSFORMATION_CMD - 1){
         setTransformationEnabled(false);
    	return;
    }

    strcpy(transformationCommand, command);
    setTransformationEnabled(true);
}

char * getTransformationCommand(){
  return transformationCommand;
}