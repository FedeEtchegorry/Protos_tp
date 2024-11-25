#include "transaction.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include "../parserUtils.h"
#include "../client/pop3Server.h"
#include "../client/pop3Parser.h"
#include "../manager/managerServer.h"
#include "../manager/managerParser.h"

//------------------------------------------------------ Private Functions ---------------------------------------------

#define MAX_AUX_BUFFER_SIZE    255
#define MAX_DIRENT_SIZE        512

static int checkNoiseArguments(struct selector_key* key){
  clientData* data = ATTACHMENT(key);
  char message[MAX_AUX_BUFFER_SIZE];
  if (data->data.parser.arg2 != NULL) {
    snprintf(message, MAX_AUX_BUFFER_SIZE, "%s %s", NOISE_ARGUMENTS, data->data.parser.arg2);
    writeInBuffer(key, true, true, message, strlen(message));
    return 1;
  }
  return 0;
}


static long int checkEmailNumber(struct selector_key* key, long int * result) {
    clientData* data = ATTACHMENT(key);
    errno=0;
    char* endPtr;
    const long int msgNumber = strtol(data->data.parser.arg, &endPtr, 10);
    if (endPtr == data->data.parser.arg || *endPtr != '\0' || errno == ERANGE) {
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

static void handleCapa(struct selector_key* key){
    const methodsMap * methods = getPop3Methods();
    char method[MAX_AUX_BUFFER_SIZE];
    for (int i = 0; methods[i].command!=NULL; i++) {
        snprintf(method, MAX_AUX_BUFFER_SIZE, "%s", methods[i].command);
        writeInBuffer(key, true, false, method, strlen(method));
    }
}

static void handleList(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    if (checkNoiseArguments(key))
        return;
    char message[MAX_AUX_BUFFER_SIZE];
    if (data->data.parser.arg == NULL) {
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
        if (data->data.parser.arg == NULL)
            data->data.parser.arg = "";

        long int msgNumber;
        if (checkEmailNumber(key, &msgNumber) == 0) {
            snprintf(message, MAX_AUX_BUFFER_SIZE, "%ld %u", msgNumber, data->mails[msgNumber - 1]->size);
            writeInBuffer(key, true, false, message, strlen(message));
        }
    }
}

static void handleRetr(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    char auxBuffer[MAX_AUX_BUFFER_SIZE];
    if (checkNoiseArguments(key))
        return;
    long int msgNumber;
    if (checkEmailNumber(key, &msgNumber) == 0) {

        snprintf(auxBuffer, MAX_AUX_BUFFER_SIZE, "%u octets", data->mails[msgNumber - 1]->size);
        writeInBuffer(key, true, false, auxBuffer, strlen(auxBuffer));

        snprintf(auxBuffer, MAX_AUX_BUFFER_SIZE, "%s/%s/%s/%s", mailDirectory, data->data.currentUsername,
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
            snprintf(oldPath, MAX_AUX_BUFFER_SIZE, "%s/%s/%s/%s", mailDirectory, data->data.currentUsername, "new",
                     data->mails[msgNumber - 1]->filename);
            snprintf(newPath, MAX_AUX_BUFFER_SIZE, "%s/%s/%s/%s", mailDirectory, data->data.currentUsername, "cur",
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
    fprintf(stdout, "Archivo: %s, función: %s, linea: %d\n", __FILE__, __func__, __LINE__);
    clientData* data = ATTACHMENT(key);
    unsigned totalOctets = 0;
    unsigned totalEmail = 0;

    fprintf(stdout, "Data: %p\n", (void*)&data->mailCount);

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

static void handleData(struct selector_key* key){
    printf("MENSAJE");
}

//--------------------------------------- Aux functions ----------------------------------------------------------------

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
    snprintf(path, MAX_AUX_BUFFER_SIZE, "%s/%s", mailDirectory, data->data.currentUsername);
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char* subdirs[] = {"new", "cur", "tmp"};
    for (int i=0; i<3 ; i++) {
        char subdir[512];
        snprintf(subdir, sizeof(subdir), "%s/%s/%s", mailDirectory, data->data.currentUsername, subdirs[i]);
        mkdir(subdir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

}

static void loadMails(clientData* data) {
    DIR* dir;
    struct dirent* entry;

    const char* subdirs[] = {"new", "cur"};

    for (int i = 0; i < 2; i++) {
        char subdirPath[MAX_AUX_BUFFER_SIZE];
        snprintf(subdirPath, MAX_AUX_BUFFER_SIZE, "%s/%s/%s", mailDirectory, data->data.currentUsername, subdirs[i]);
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

//------------------------------------------------------ Public Functions For Pop3 -------------------------------------

void transactionOnArrival(const unsigned int state, struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    createMaildir(data);
    loadMails(data);
    selector_set_interest_key(key, OP_READ);
}

unsigned transactionOnReadReady(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    switch (data->data.parser.method) {
    case CAPA:
        handleCapa(key);
        break;
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
        break;
    default:
        handleUnknown(key);
    }
    return TRANSACTION;
}

//------------------------------------------------- Public Functions For Manager ---------------------------------------

void transactionOnArrivalForManager(const unsigned int state, struct selector_key* key){
    selector_set_interest_key(key, OP_READ);
}

unsigned transactionManagerOnReadReady(struct selector_key* key) {
    managerData* data = ATTACHMENT_MANAGER(key);
    switch (data->manager_data.parser.method) {
    case DATA:
        //get_stored_data();
        break;
    case QUIT_M:
        return MANAGER_EXIT;
    case CAPA_M:
        handleCapa(key);
        break;
    default:
        handleUnknown(key);
    }
    return MANAGER_TRANSACTION;
}
