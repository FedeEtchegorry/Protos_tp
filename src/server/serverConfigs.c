#include "serverConfigs.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>


bool serverBlocked = false;
bool transformationEnabled = false;
char * transformationCommand = NULL;

void setServerBlocked(bool block) {
    serverBlocked = block;
}
bool isServerBlocked() {
    return serverBlocked;
}


void setTransformationEnabled(bool enabled) {
    transformationEnabled = enabled;
}

bool isTransformationEnabled() {
    return transformationEnabled;
}

void setTransformationCommand(const char* command){
  	if(command == NULL || strlen(command) > MAX_SIZE_TRANSFORMATION_CMD || command[0] == '\0'){
         setTransformationEnabled(false);
    	return;
    }
    transformationCommand = malloc(strlen(command) + 1);
    strcpy(transformationCommand, command);
    setTransformationEnabled(true);
}

char * getTransformationCommand(){
  return transformationCommand;
}