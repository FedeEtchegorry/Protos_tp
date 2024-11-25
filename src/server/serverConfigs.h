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
#define NOISE_ARGUMENTS                   "Noise after message number"
#define NEW_USER_ARGUMENT_REQUIRED       "Argument required: <username:password>"
#define ILLEGAL_USERNAME_OR_PASSWORD     "Username and password cannot be empty"
#define ERROR_ADDING_USER                "Error adding user"
#define EMPTY_USERNAME_DELETE            "Please provide an username"
#define ERROR_DELETING_USER              "Error deleting user"
#define ERROR_BLOCKING_USER              "Error blocking/unblocking user"
#define ERROR_MAKING_USER_ADMIN          "Error making user admin"

// --------------------------------- MANAGER ---------------------------------------------------------------------------

#define HISTORIC_DATA_FILE               "historic.csv"

// ---------------------------------------------------------------------------------------------------------------------

#define DEBUG_PRINT_LOCATION()          fprintf(stdout, "Archivo: %s, Funcion: %s, Linea: %d\n", __FILE__, __func__, __LINE__)


typedef struct {

    size_t ioReadBufferSize;
    size_t ioWriteBufferSize;

} server_configuration;

void setServerBlocked(bool block);
bool isServerBlocked();
#endif // SERVER_CONFIGS_H