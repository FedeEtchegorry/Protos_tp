#include "transaction.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include "users.h"
#include "../parserUtils.h"
#include "../client/pop3Server.h"
#include "../client/pop3Parser.h"
#include "../manager/managerServer.h"
#include "../manager/managerParser.h"
#include "../logging/metrics.h"
#include "../logging/logger.h"
#include <sys/wait.h>
#include "../serverConfigs.h"
#include "selector.h"


extern server_metrics *clientMetrics;
extern server_logger *logger;

//------------------------------------------------------ Private Functions ---------------------------------------------

#define MAX_AUX_BUFFER_SIZE    255
#define MAX_DIRENT_SIZE        512

void handlePipeRead(struct selector_key* key);

static const fd_handler pipeHandler = {
    .handle_read = handlePipeRead,  // Función de lectura del pipe
    .handle_write = NULL,           // No necesitamos manejar escritura
    .handle_close = NULL,           // No necesitamos manejar cierre
};


static int checkNoiseArguments(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    char message[MAX_AUX_BUFFER_SIZE];
    char * extraArg = parserGetExtraArg(data->data.parser);
    if (extraArg != NULL) {
        snprintf(message, MAX_AUX_BUFFER_SIZE, "%s: %s", NOISE_ARGUMENTS, extraArg);
        writeInBuffer(key, true, true, message, strlen(message));
        return 1;
    }
    return 0;
}

static void handleCapa(struct selector_key* key) {
    char method[MAX_AUX_BUFFER_SIZE];
    snprintf(method, MAX_AUX_BUFFER_SIZE, "%s", "Capability list follows");
    writeInBuffer(key, true, false, method, strlen(method));
    snprintf(method, MAX_AUX_BUFFER_SIZE, "%s", "UIDL");
    writeInBuffer(key, false, false, method, strlen(method));
    snprintf(method, MAX_AUX_BUFFER_SIZE, "%s", "PIPELINING");
    writeInBuffer(key, false, false, method, strlen(method));
    writeInBuffer(key, false, false, ".", strlen("."));
}

static long int checkEmailNumber(struct selector_key* key, long int* result) {
    clientData* data = ATTACHMENT(key);
    errno = 0;
    char* endPtr;
    char * arg = parserGetFirstArg(data->data.parser);
    if(arg == NULL) {
        writeInBuffer(key, true, true, MISSING_ARGUMENT, sizeof(MISSING_ARGUMENT) - 1);
        return 4;
    }
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

static long long unsigned int generateUIDL(const char* filename) {
    unsigned long long hash = 932280971;

    for (const char* ptr = filename; *ptr; ptr++) {
        hash = (hash * 31) + (unsigned char)*ptr;
    }

    return hash * strlen(filename);
}

static void handleUIDL(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    char message[MAX_AUX_BUFFER_SIZE];
    if (checkNoiseArguments(key)) {
        return;
    }

    char * arg = parserGetFirstArg(data->data.parser);
    if (arg == NULL) {
        writeInBuffer(key, true, false, "", strlen(""));
        for (long int i = 0; i < data->mailCount; i++) {
            if (!(data->mails[i]->deleted)) {
                long long unsigned int uidl = data->mails[i]->checksum;
                sprintf(message, "%li %llu", i + 1, uidl);
                writeInBuffer(key, false, false, message, strlen(message));
            }
        }
        sprintf(message, ".");
        writeInBuffer(key, false, false, message, strlen(message));
    } else {
        long int mail_num;
        if (checkEmailNumber(key, &mail_num) == 0) {
            long unsigned int uidl = data->mails[mail_num - 1]->checksum;
            sprintf(message, "%ld %lu", mail_num, uidl);
            writeInBuffer(key, true, false, message, strlen(message));
        }
    }
}

static void handleList(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    if (checkNoiseArguments(key))
        return;
    char message[MAX_AUX_BUFFER_SIZE];
    char * arg = parserGetFirstArg(data->data.parser);
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
    } else {
        long int msgNumber;
        if (checkEmailNumber(key, &msgNumber) == 0) {
            snprintf(message, MAX_AUX_BUFFER_SIZE, "%ld %u", msgNumber, data->mails[msgNumber - 1]->size);
            writeInBuffer(key, true, false, message, strlen(message));
        }
    }
}

typedef struct {
    int pipefd;  // Pipe
    struct selector_key * key;
    int pid;
} child_process_data;

void handlePipeRead(struct selector_key* key) {
    // Acceder a la estructura con el pid
    child_process_data* processData = (child_process_data*)key->data;

    char buffer[MAX_SIZE_TRANSFORMATION_BUFFER];
    printf("Antes del read\n");
    ssize_t bytesRead = read(processData->pipefd, buffer, sizeof(buffer) - 1);
    printf("Bytes leidos: %zd,z Buffer: %s\n", bytesRead, buffer);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        printf("Se leyeron del buffer %zd bytes: %s\n", bytesRead, buffer);
        writeInBuffer(key, false, false, buffer, bytesRead);
    } else if (bytesRead == 0) {
        printf("Pipe is empty\n");

        if (waitpid(processData->pid, NULL, WNOHANG) > 0) {
            printf("Proceso hijo ha terminado\n");
            free(processData);
        }
    } else {
        fprintf(stderr, "Error reading from pipe: %s\n", strerror(errno));
    }
}


static void transform(struct selector_key* key, const char *src) {
    int pipefd[2];

    // Crear el pipe
    if (pipe(pipefd) < 0) {
        perror("Error creating pipe");
        return;
    }

    // Crear el proceso hijo
    int pid = fork();
    if (pid < 0) {
        perror("Error creating child");
        return;
    }

    if (pid == 0) {
        // Proceso hijo: redirigir stdout al pipe
        close(1);  // Cerrar stdout
        if (dup(pipefd[1]) < 0) {
            perror("Error duplicating file descriptor");
            exit(EXIT_FAILURE);
        }
        close(pipefd[0]);  // No necesitamos el descriptor de lectura

        // Ejecutar el comando de transformación
        char fullCommand[MAX_SIZE_TRANSFORMATION_CMD];
        snprintf(fullCommand, sizeof(fullCommand), "/bin/%s", getTransformationCommand());
        char* args[] = {fullCommand, (char*)src, NULL};
        execve(fullCommand, args, NULL);

        // Si execve falla
        perror("Execve failed");
        exit(EXIT_FAILURE);
    }

    // Proceso padre: cerrar el descriptor de escritura del pipe
    close(pipefd[1]);

    clientData * client_data = ATTACHMENT(key);

    child_process_data* processData = malloc(sizeof(child_process_data));
    processData->pipefd = pipefd[0];
    processData->key = key;
    processData->pid = pid;

    //key->data = processData;

    // Definir el manejador de eventos

    key->fd = pipefd[0];  // El descriptor de archivo es el del extremo de lectura

    selector_status status = selector_register(key->s, pipefd[0], &pipeHandler, OP_READ, processData);
    if (status != SELECTOR_SUCCESS) {
        fprintf(stderr, "Error registering pipe fd with selector\n");
        close(pipefd[0]);
        free(processData);  // Liberar memoria
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

        if(isTransformationEnabled()){
            transform(key, auxBuffer);
        }
        else
        {
            FILE* mail = fopen(auxBuffer, "r");
            while (fgets(auxBuffer, MAX_AUX_BUFFER_SIZE, mail) != NULL)
            {
                auxBuffer[strlen(auxBuffer) - 1] = '\0';
                writeInBuffer(key, false, false, auxBuffer, strlen(auxBuffer));
            }
            fclose(mail);
        }

        writeInBuffer(key, false, false, ".", 1);

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

static void handleAddUser(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    char * arg = parserGetFirstArg(data->data.parser);
    if (arg == NULL) {
        writeInBuffer(key, true, true, NEW_USER_ARGUMENT_REQUIRED, sizeof(NEW_USER_ARGUMENT_REQUIRED) - 1);
        return;
    }
    char* username = strtok(arg, ":");
    char* password = strtok(NULL, ":");

    if (username == NULL || password == NULL) {
        writeInBuffer(key, true, true, ILLEGAL_USERNAME_OR_PASSWORD, sizeof(ILLEGAL_USERNAME_OR_PASSWORD) - 1);
        return;
    }

    if (addUser(username, password, ROLE_USER)) {
        writeInBuffer(key, true, false, NULL, 0);
    }
    else {
        writeInBuffer(key, true, true, ERROR_ADDING_USER, sizeof(ERROR_ADDING_USER) - 1);
    }
}

static void handleBlock(struct selector_key* key, bool block) {
    setServerBlocked(block);
    writeInBuffer(key, true, false, NULL, 0);
}

static void handleSudo(struct selector_key* key) {
    clientData* data = ATTACHMENT(key);
    char * arg = parserGetFirstArg(data->data.parser);
    if (arg == NULL) {
        writeInBuffer(key, true, true, EMPTY_USERNAME_DELETE, sizeof(EMPTY_USERNAME_DELETE) - 1);
        return;
    }
    char* username = arg;

    if (makeUserAdmin(username)) {
        writeInBuffer(key, true, false, NULL, 0);
    }
    else {
        writeInBuffer(key, true, true, ERROR_MAKING_USER_ADMIN, sizeof(ERROR_MAKING_USER_ADMIN) - 1);
    }
}


static void handlerRst(struct selector_key* key) {
    clientMetrics->totalCountConnections = 0;
    clientMetrics->totalTransferredBytes = 0;
    clientMetrics->totalReceivedBytes = 0;
    writeInBuffer(key, true, false, NULL, 0);
}

static void handleData(struct selector_key* key) {
    char buffer[1024];

    int bytesWritten = snprintf(buffer, sizeof(buffer),
                                "\nServer Metrics:\n"
                                "-------------------------\n"
                                "Total Count Connections: %zu\n"
                                "Current Connections Count: %zu\n"
                                "Total Transferred Bytes: %zu\n"
                                "Total Received Bytes: %zu\n"
                                "Total Count Users: %zu\n"
                                "IO Read Buffer Size: %zu bytes\n"
                                "IO Write Buffer Size: %zu bytes\n"
                                "Data File Path: %s",
                                clientMetrics->totalCountConnections,
                                clientMetrics->currentConectionsCount,
                                clientMetrics->totalTransferredBytes,
                                clientMetrics->totalReceivedBytes,
                                clientMetrics->totalCountUsers,
                                clientMetrics->ioReadBufferSize ? *clientMetrics->ioReadBufferSize : 0,
                                clientMetrics->ioWriteBufferSize ? *clientMetrics->ioWriteBufferSize : 0,
                                clientMetrics->dataFilePath ? clientMetrics->dataFilePath : "(not set)"
    );

    if (bytesWritten < 0) {
        fprintf(stderr, "Error formatting server metrics\n");
    }
    else if ((size_t)bytesWritten >= sizeof(buffer)) {
        fprintf(stderr, "Buffer overflow detected when formatting server metrics\n");
    }
    else {
        writeInBuffer(key, true, false, buffer, bytesWritten);
    }
}

static void handlerGetLog(struct selector_key* key) {
	clientData* data = ATTACHMENT(key);
    char buffer[1024];
    unsigned long lines = 0;
	unsigned long bytesWritten = 0;

    if (logger == NULL) {
        bytesWritten = snprintf(buffer, sizeof(buffer), "Logs are disabled\n");
    }
    else {
        buffer[0] = '\n';

        char * arg = parserGetFirstArg(data->data.parser);
        if (arg) {
        	lines = strtol(arg, NULL, 10);
        }
    	bytesWritten = serverLoggerRetrieve(logger, buffer+1, sizeof(buffer)-1, &lines) + 1;
    }
    writeInBuffer(key, true, false, buffer, bytesWritten);
}

static void handlerEnableLog(struct selector_key* key) {

	clientData* data = ATTACHMENT(key);

    char buffer[128];
    unsigned long enable = 0;
	unsigned long bytesWritten = 0;

    char * arg = parserGetFirstArg(data->data.parser);
    if (arg) {

        enable = strtol(arg, NULL, 10);

        if (enable == 0 && logger == NULL) {
          	bytesWritten = snprintf(buffer, sizeof(buffer), "Logs already disabled\n");
        }
        else if (enable == 0) {
          	snprintf(buffer, sizeof(buffer), "Logs beeing disabled by '%s'", data->data.currentUsername);
            serverLoggerRegister(logger, buffer);
          	serverLoggerTerminate(&logger);
          	bytesWritten = snprintf(buffer, sizeof(buffer), "Logs disabled\n");
        }
        else if (logger != NULL) {
          	bytesWritten = snprintf(buffer, sizeof(buffer), "Logs already enabled\n");
        }
        else {
			logger = serverLoggerCreate(&key->s, LOG_DATA_FILE);

            if (logger != NULL) {
            	snprintf(buffer, sizeof(buffer), "Logs enabled by '%s'\n", data->data.currentUsername);
                serverLoggerRegister(logger, buffer);
            }
            else {
                bytesWritten = snprintf(buffer, sizeof(buffer), "I was not enable, sorry\n");
        	}
        }
    }
    else {
      	strcpy(buffer, MISSING_ARGUMENT);
        bytesWritten = sizeof(MISSING_ARGUMENT) - 1;
    }

    writeInBuffer(key, true, false, buffer, bytesWritten);
}

static void handlerEnableTransformation(struct selector_key* key)
{
    clientData* data = ATTACHMENT(key);
    char * arg = parserGetFirstArg(data->data.parser);

    if (arg == NULL) {
        writeInBuffer(key, true, true, INVALID_TRANSFORMATION_ARGUMENT, sizeof(INVALID_TRANSFORMATION_ARGUMENT) - 1);
        return;
    }

    size_t num = strtol(arg, NULL, 10);
    if(num != 0 && num != 1)
    {
        writeInBuffer(key, true, true, INVALID_TRANSFORMATION_ARGUMENT, sizeof(INVALID_TRANSFORMATION_ARGUMENT) - 1);
        return;
    }

    if(num == 1)
    {
        if(!setTransformationEnabled(true))
        {
            writeInBuffer(key, true, true, ERROR_SET_TRANSF_FIRST, sizeof(ERROR_SET_TRANSF_FIRST) - 1);
        }
    }
    else
    {
        setTransformationEnabled(false);
    }
    writeInBuffer(key, true, false, NULL, 0);
}

static void handleSetTransformation(struct selector_key* key)
{
    clientData* data = ATTACHMENT(key);
    char * arg = parserGetFirstArg(data->data.parser);

    if(arg == NULL)
    {
        writeInBuffer(key, true, true, MISSING_ARGUMENT, sizeof(MISSING_ARGUMENT) - 1);
        return;
    }
    int result = setTransformationCommand(arg);
    switch (result) {
    case SUCCESS:
        writeInBuffer(key, true, false, NULL, 0);
        break;
    case ERR_NULL_COMMAND:
        writeInBuffer(key, true, true, TRANSFORM_ERR_NULL_COMMAND, sizeof(TRANSFORM_ERR_NULL_COMMAND)-1);
        break;
    case ERR_COMMAND_TOO_LONG:
        writeInBuffer(key, true, true, TRANSFORM_ERR_COMMAND_TOO_LONG, sizeof(TRANSFORM_ERR_COMMAND_TOO_LONG)-1);
        break;

    case ERR_EMPTY_COMMAND:
        writeInBuffer(key, true, true, TRANSFORM_ERR_EMPTY_COMMAND, sizeof(TRANSFORM_ERR_EMPTY_COMMAND)-1);
        break;
    default:
        writeInBuffer(key, true, true, TRANSFORM_ERR_INVALID_PATH, sizeof(TRANSFORM_ERR_INVALID_PATH)-1);
        break;
    }
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

static void createMaildir(clientData* data) {
    char path[MAX_AUX_BUFFER_SIZE];
    snprintf(path, MAX_AUX_BUFFER_SIZE, "%s/%s", mailDirectory, data->data.currentUsername);
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    const char* subdirs[] = {"new", "cur", "tmp"};
    for (int i = 0; i < 3; i++) {
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
            info->checksum = generateUIDL(info->filename);

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
    switch (parserGetMethod(data->data.parser)) {
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
    case CAPA:
        handleCapa(key);
        break;
    case UIDL:
        handleUIDL(key);
        break;
    default:
        handleUnknown(key);
    }
    return TRANSACTION;
}

//------------------------------------------------- Public Functions For Manager ---------------------------------------

void transactionOnArrivalForManager(const unsigned int state, struct selector_key* key) {
    selector_set_interest_key(key, OP_READ);
}

unsigned transactionManagerOnReadReady(struct selector_key* key) {
    managerData* data = ATTACHMENT_MANAGER(key);
    switch (parserGetMethod(data->manager_data.parser)) {
    case DATA:
        handleData(key);
        break;
    case ADDUSER:
        handleAddUser(key);
        break;
    case BLOCK:
        handleBlock(key, true);
        break;
    case UNBLOCK:
        handleBlock(key, false);
        break;
    case SUDO:
        handleSudo(key);
        break;
    case RST:
        handlerRst(key);
        break;
    case LOGG:
        handlerGetLog(key);
        break;
    case ENLOG:
        handlerEnableLog(key);
        break;
    case SETTR:
        handleSetTransformation(key);
        break;
    case ENTR:
        handlerEnableTransformation(key);
        break;
    case QUIT_M:
        return MANAGER_EXIT;
    default:
        handleUnknown(key);
    }
    return MANAGER_TRANSACTION;
}
