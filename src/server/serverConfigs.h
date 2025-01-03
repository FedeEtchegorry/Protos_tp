#ifndef SERVER_CONFIGS_H
#define SERVER_CONFIGS_H

#include <unistd.h>
#include<stdbool.h>
// ----------------------------------- MAIN ----------------------------------------------------------------------------

#define DEFAULT_PORT                    1080
#define SELECTOR_SIZE                   1024

#define DEFAULT_IO_BUFFER_SIZE          8096
#define MAX_PENDING_CONNECTION_REQUESTS 5


// ----------------------------------- USERS ---------------------------------------------------------------------------

#define MAX_USERS                        10
#define MAX_MAILS                        50
#define MAX_SIZE_TRANSFORMATION_CMD      50
#define USERS_CSV                        "users.csv"
#define USERS_MAX_USERNAME_LENGTH        254
#define USERS_MAX_PASSWORD_LENGTH        254
#define DEFAULT_ADMIN_USERNAME            "admin"
#define DEFAULT_ADMIN_PASSWORD            "admin"

#define SUCCESS_MSG                      "+OK "
#define ERROR_MSG                        "-ERR "

#define GREETING                         "POP3 server ready"
#define NO_MESSAGE_FOUND                 "No message found"
#define INVALID_NUMBER                   "Invalid message number"
#define INVALID_COMMAND                  "Unknown command"
#define LOG_OUT                          "Logging out"
#define AUTH_FAILED                      "Authentication failed"
#define AUTH_SUCCESS                     "Logged in successfully"
#define NO_USERNAME                      "No username given"
#define INVALID_METHOD                   "Invalid method"
#define MESSAGE_DELETED                  "Message deleted"
#define NOISE_ARGUMENTS                  "Noise after message number"
#define NEW_USER_ARGUMENT_REQUIRED       "Argument required: <username:password>"
#define ILLEGAL_USERNAME_OR_PASSWORD     "Username and password cannot be empty"
#define ERROR_ADDING_USER                "Error adding user"
#define EMPTY_USERNAME_DELETE            "Please provide an username"
#define ERROR_DELETING_USER              "Error deleting user"
#define ERROR_BLOCKING_USER              "Error blocking/unblocking user"
#define ERROR_MAKING_USER_ADMIN          "Error making user admin"
#define INVALID_ARGUMENT                 "Invalid argument"
#define MISSING_ARGUMENT                 "Missing argument"
#define INVALID_TRANSFORMATION_ARGUMENT  "Invalid argument. Value must be 0 to disable transformations or 1 to enable them"
#define ERROR_SET_TRANSF_FIRST           "Cannot enable transformations without setting a command first. Try using 'SETTR <command>' first"
#define USER_ALREADY_CONNECTED           "Authentication failed. User is already connected"
#define SERVER_BLOCKED                   "Server blocked"

// --------------------------------- MANAGER ---------------------------------------------------------------------------

#define HISTORIC_DATA_FILE               "historic.csv"
#define LOG_DATA_FILE                    "log.txt"
#define LOG_RETRIEVE_MAX_LINES            64
#define LOG_DEFAULT_ENABLED               1

// ---------------------------------------------------------------------------------------------------------------------

#define DEBUG_PRINT_LOCATION()          fprintf(stdout, "Archivo: %s, Funcion: %s, Linea: %d\n", __FILE__, __func__, __LINE__)


typedef struct {

    size_t ioReadBufferSize;
    size_t ioWriteBufferSize;

} server_configuration;

bool setTransformationEnabled(bool enabled);
void setTransformationCommand(const char* command);
char * getTransformationCommand();
bool isTransformationEnabled();
void setServerBlocked(bool block);
bool isServerBlocked();
char* getMailDirPath();
#endif // SERVER_CONFIGS_H