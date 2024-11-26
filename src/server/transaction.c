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

//------------------------------------------------------Private Functions------------------------------------------

#define MAX_AUX_BUFFER_SIZE 255
#define MAX_DIRENT_SIZE 512

static long int checkEmailNumber(struct selector_key* key, long int * result) {
    clientData* data = ATTACHMENT(key);
    errno=0;
    char* endPtr;
    char * arg = parserGetArg(data->pop3Parser);
    const long int msgNumber = strtol(arg, &endPtr, 10);
    if (endPtr == arg || *endPtr != '\0' || errno == ERANGE) {
        writeInBuffer(key, true, true, INVALID_NUMBER, sizeof(INVALID_NUMBER) - 1);
        return 1;
    }
    if (msgNumber > data->mailCount || msgNumber <= 0) {
        writeInBuffer(key, true, true, NO_MESSAGE_FOUND, sizeof(NO_MESSAGE_FOUND) - 1);
        return 2;
    }
    if (data->mails[msgNumber - 1]->deleted) {
        writeInBuffer(key, true, true, MESSAGE_DELETED, sizeof(MESSAGE_DELETED) - 1);
        return 3;
    }
    *result = msgNumber;
    return 0;
}

static void handleList(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    char message[MAX_AUX_BUFFER_SIZE];
    char * arg = parserGetArg(data->pop3Parser);
    if (arg == NULL) {
        unsigned notDeleted = 0;
        for (unsigned i = 0; i < data->mailCount; i++) {
            if (!data->mails[i]->deleted)
                notDeleted++;
        }

        snprintf(message, MAX_AUX_BUFFER_SIZE, "%u messages:", notDeleted);
        writeInBuffer(key, true, false, message, strlen(message));

        for (unsigned i = 0; i < data->mailCount; i++) {
            if (!data->mails[i]->deleted) {
                snprintf(message, sizeof(message), "%u %u", i + 1, data->mails[i]->size);
                writeInBuffer(key, false, false, message, strlen(message));
            }
        }

        writeInBuffer(key, false, false, ".", 1);
    }
    else {
        long int msgNumber;
        if (checkEmailNumber(key, &msgNumber) == 0) {
            snprintf(message, MAX_AUX_BUFFER_SIZE, "%ld %u", msgNumber, data->mails[msgNumber - 1]->size);
            writeInBuffer(key, true, false, message, strlen(message));
        }
    }
}

static void handleRetr(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);

    long int msgNumber;
    if (checkEmailNumber(key, &msgNumber) == 0) {
        char auxBuffer[MAX_AUX_BUFFER_SIZE];
        snprintf(auxBuffer, MAX_AUX_BUFFER_SIZE, "%u octets", data->mails[msgNumber - 1]->size);
        writeInBuffer(key, true, false, auxBuffer, strlen(auxBuffer));

        snprintf(auxBuffer, MAX_AUX_BUFFER_SIZE, "%s/%s/%s/%s", mailDirectory, data->currentUsername,
                 data->mails[msgNumber - 1]->seen ? "cur" : "new", data->mails[msgNumber - 1]->filename);
        FILE* mail = fopen(auxBuffer, "r");


        while (fgets(auxBuffer, MAX_AUX_BUFFER_SIZE, mail) != NULL) {
            auxBuffer[strlen(auxBuffer) - 1] = '\0';
            writeInBuffer(key, false, false, auxBuffer, strlen(auxBuffer));
        }

        writeInBuffer(key, false, false, ".", 1);

        fclose(mail);

        if (data->mails[msgNumber - 1]->seen == false) {
            data->mails[msgNumber - 1]->seen = true;
            char oldPath[MAX_AUX_BUFFER_SIZE];
            char newPath[MAX_AUX_BUFFER_SIZE];
            snprintf(oldPath, MAX_AUX_BUFFER_SIZE, "%s/%s/%s/%s", mailDirectory, data->currentUsername, "new",
                     data->mails[msgNumber - 1]->filename);
            snprintf(newPath, MAX_AUX_BUFFER_SIZE, "%s/%s/%s/%s", mailDirectory, data->currentUsername, "cur",
                     data->mails[msgNumber - 1]->filename);
            rename(oldPath, newPath);
        }
    }
}

static void handleRset(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    for (unsigned i = 0; i < data->mailCount; i++)
        data->mails[i]->deleted = false;
    writeInBuffer(key, true, false, NULL, 0);
}

static void handleNoop(struct selector_key* key) {
    writeInBuffer(key, true, false, NULL, 0);
}

static void handleDelete(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    long int msgNumber;
    if (checkEmailNumber(key, &msgNumber) == 0) {
        data->mails[msgNumber - 1]->deleted = true;
        writeInBuffer(key, true, false, NULL, 0);
    }
}

static void handleStat(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    unsigned totalOctets = 0;
    unsigned totalEmail = 0;
    for (unsigned i = 0; i < data->mailCount; i++)
        if (!data->mails[i]->deleted) {
            totalEmail++;
            totalOctets += data->mails[i]->size;
        }

    char auxBuffer[MAX_AUX_BUFFER_SIZE];
    snprintf(auxBuffer, MAX_AUX_BUFFER_SIZE, "%u %u", totalEmail, totalOctets);
    writeInBuffer(key, true, false, auxBuffer, strlen(auxBuffer));
}

static void handleUnknown(struct selector_key* key) {
    writeInBuffer(key, true, true, INVALID_COMMAND, sizeof(INVALID_COMMAND) - 1);
}


//---------------------------------------Aux functions-------------------------------------------------
static size_t calculateOctetLength(const char* filePath) {
    FILE* file = fopen(filePath, "rb");

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

static void createMaildir(clientData * data) {
    char path[MAX_AUX_BUFFER_SIZE];
    snprintf(path, MAX_AUX_BUFFER_SIZE, "%s/%s", mailDirectory, data->currentUsername);
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char* subdirs[] = {"new", "cur", "tmp"};
    for (int i=0; i<3 ; i++) {
        char subdir[512];
        snprintf(subdir, sizeof(subdir), "%s/%s/%s", mailDirectory, data->currentUsername, subdirs[i]);
        mkdir(subdir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

}

static void loadMails(clientData* data) {
    DIR* dir;
    struct dirent* entry;

    const char* subdirs[] = {"new", "cur"};

    for (int i = 0; i < 2; i++) {
        char subdirPath[MAX_AUX_BUFFER_SIZE];
        snprintf(subdirPath, MAX_AUX_BUFFER_SIZE, "%s/%s/%s", mailDirectory, data->currentUsername, subdirs[i]);

        dir = opendir(subdirPath);
        if (dir == NULL) {
            perror("opendir failed");
            continue;
        }

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            struct mailInfo* info = malloc(sizeof(struct mailInfo));

            char mailPath[512];
            snprintf(mailPath, sizeof(mailPath), "%s/%s", subdirPath, entry->d_name);

            info->filename = strdup(entry->d_name);
            info->size = calculateOctetLength(mailPath);
            info->seen = (i == 1);
            info->deleted = false;

            data->mails[data->mailCount++] = info;
        }

        closedir(dir);
    }
}

//------------------------------------------------------Public Functions------------------------------------------
void transactionOnArrival(const unsigned int state, struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    createMaildir(data);
    loadMails(data);
    selector_set_interest_key(key, OP_READ);
}

unsigned transactionOnReadReady(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    switch (parserGetMethod(data->pop3Parser)) {
    case LIST:
        handleList(key);
        break;
    case RETR:
        handleRetr(key);
        break;
    case RSET:
        handleRset(key);
        break;
    case NOOP:
        handleNoop(key);
        break;
    case DELE:
        handleDelete(key);
        break;
    case STAT:
        handleStat(key);
        break;
    case QUIT:
        return UPDATE;
    default:
        handleUnknown(key);
    }

    return TRANSACTION;
}

