#include "transaction.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>

#include "POP3Server.h"

#define NO_MESSAGE_FOUND "No message found"
#define INVALID_NUMBER "Invalid message number"

const char * mailDirectory = NULL;

//------------------------------------------------------Private Functions------------------------------------------
#define MAX_AUX_BUFFER_SIZE 255
static void handleList(struct selector_key * key) {
    clientData* data = ATTACHMENT(key);
    char message[MAX_AUX_BUFFER_SIZE];

    if (data->pop3Parser.arg == NULL) {
        snprintf(message, MAX_AUX_BUFFER_SIZE, "%u messages:", data->mailCount);
        writeInBuffer(key, true, false, message, strlen(message));

        for (unsigned i = 0; i < data->mailCount; i++) {
            if (!data->mails[i]->deleted) {
                snprintf(message, sizeof(message), "%u %u", i + 1, data->mails[i]->size);
                writeInBuffer(key, false, false, message, strlen(message));
            }
        }

        writeInBuffer(key, false, false, ".", 1);
    } else {
        char * endPtr;
        errno=0;
        long int msgNumber = strtol(data->pop3Parser.arg, &endPtr, 10);
        if (endPtr == data->pop3Parser.arg || *endPtr != '\0' || errno == ERANGE) {
            writeInBuffer(key, true, true, INVALID_NUMBER, sizeof(INVALID_NUMBER)-1);
        } else if (msgNumber > data->mailCount || msgNumber < 0) {
            writeInBuffer(key, true, true, NO_MESSAGE_FOUND, sizeof(NO_MESSAGE_FOUND)-1);
        }else{
            snprintf(message, MAX_AUX_BUFFER_SIZE, "%ld %u", msgNumber, data->mails[msgNumber-1]->size);
            writeInBuffer(key, true, false, message, strlen(message));
        }
    }
}
static unsigned handleRetr(struct selector_key * key) {

}
static unsigned handleRset(struct selector_key * key) {

}
static unsigned handleNoop(struct selector_key * key) {

}
static unsigned handleDelete(struct selector_key * key) {

}
static unsigned handleStat(struct selector_key * key) {

}
static unsigned handleQuit(struct selector_key * key) {

}
static unsigned handleUnknown(struct selector_key * key) {

}

static size_t calculateOctetLength(const char *filePath) {
    FILE *file = fopen(filePath, "rb");

    size_t octetCount = 0;
    int prevChar = '\0';
    int currentChar;

    while ((currentChar = fgetc(file)) != EOF) {
        octetCount++;

        if (currentChar == '\n' && prevChar != '\r') {
            octetCount++;
        }

        if (prevChar == '\n' || prevChar == '\0') {
            if (currentChar == '.') {
                octetCount++;
            }
        }

        prevChar = currentChar;
    }

    fclose(file);
    return octetCount;
}

static void loadMails(clientData * data) {
    DIR *dir;
    struct dirent *entry;

    const char *subdirs[] = { "new", "cur" };

    for (int i = 0; i < 2; i++) {
        char subdirPath[255];
        snprintf(subdirPath, sizeof(subdirPath), "%s/%s/%s", mailDirectory, data->currentUsername, subdirs[i]);

        dir = opendir(subdirPath);

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            struct mailInfo * info = malloc(sizeof(struct mailInfo));

            char mailPath[512];
            snprintf(mailPath, sizeof(mailPath), "%s/%s", subdirPath, entry->d_name);

            info->filename = strdup(entry->d_name);
            info->size = calculateOctetLength(mailPath);
            info->seen = i == 1;
            info->deleted = false;

            data->mails[data->mailCount++] = info;
        }

        closedir(dir);
    }

}

//------------------------------------------------------Public Functions------------------------------------------
void transactionOnArrival(const unsigned int state, struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    loadMails(data);
    resetParser(&data->pop3Parser);
    selector_set_interest_key(key, OP_READ);
}

unsigned transactionOnReadReady(struct selector_key* key) {
    if (readAndParse(key)) {
        clientData* data = ATTACHMENT(key);
        switch (data->pop3Parser.method) {
            case LIST:
                handleList(key);
            case RETR:
                handleRetr(key);
            case RSET:
                handleRset(key);
            case NOOP:
                handleNoop(key);
            case DELE:
                handleDelete(key);
            case STAT:
                handleStat(key);
            case QUIT:
                handleQuit(key);
            default:
                handleUnknown(key);
        }
        selector_set_interest_key(key, OP_WRITE);
    }
    return TRANSACTION;
}

unsigned transactionOnWriteReady(struct selector_key* key) {
    if (sendFromBuffer(key)) {
        clientData* data = ATTACHMENT(key);
        resetParser(&data->pop3Parser);
        selector_set_interest_key(key, OP_READ);
    }

    return TRANSACTION;
}

void initTransactionModule(const char * mailDir) {
    mailDirectory = strdup(mailDir);
}