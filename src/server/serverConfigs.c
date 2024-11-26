#include "serverConfigs.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


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

char* getMailDirPath() {
  static char newPath[2048];
  char cwd[1023];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stdout, "getcwd failed");
        return NULL;
  }
  char pathCopy[1024];
  strncpy(pathCopy, cwd, sizeof(pathCopy) - 1);
  pathCopy[sizeof(pathCopy) - 1] = '\0';
  snprintf(newPath, sizeof(newPath), "%s/mailDir", pathCopy);
  return newPath;
}