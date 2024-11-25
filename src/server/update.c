#include "update.h"
#include <stdio.h>
#include "./client/pop3Server.h"
#include "./manager/managerServer.h"

#define MAX_AUX_BUFFER_SIZE 255

void updateOnArrival(const unsigned int state, struct selector_key *key) {
  clientData * data = ATTACHMENT(key);
  int mails_deleted=0;
  if (data->mailCount != 0) {
    for (unsigned int i = 0; i < data->mailCount; i++) {
      if (data->mails[i]->deleted) {
        mails_deleted++;
        char path[MAX_AUX_BUFFER_SIZE];
        snprintf(path, MAX_AUX_BUFFER_SIZE, "%s/%s/%s/%s", mailDirectory, data->data.currentUsername, data->mails[i]->seen ? "cur":"new", data->mails[i]->filename);
        remove(path);
      }
    }
  }

  if (mails_deleted) {
    char messages[] = ", messages deleted";
    char result[strlen(messages) + strlen(LOG_OUT) + 1];
    strcpy(result, LOG_OUT);
    strcat(result, messages);
    writeInBuffer(key, true, false, result, sizeof(result)-1);
  }
  else writeInBuffer(key, true, false, LOG_OUT, sizeof(LOG_OUT)-1);


  selector_set_interest_key(key, OP_WRITE);
}

void exitOnArrival(const unsigned int state, struct selector_key *key){
  writeInBuffer(key, true, false, LOG_OUT, sizeof(LOG_OUT)-1);
  selector_set_interest_key(key, OP_WRITE);
}
