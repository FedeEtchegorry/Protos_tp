#ifndef SERVER_CONFIGS_H
#define SERVER_CONFIGS_H

// ----------------------------- MAIN -----------------------------------------------------------------------------------

#define DEFAULT_PORT                    1080
#define SELECTOR_SIZE                   1024

#define DEFAULT_BUFFER_SIZE             1024
#define MAX_PENDING_CONNECTION_REQUESTS 5


// ----------------------------- USERS ---------------------------------------------------------------------------------

#define MAX_USERS                        10
#define USERS_CSV                        "users.csv"
#define USERS_MAX_USERNAME_LENGTH        254
#define USERS_MAX_PASSWORD_LENGTH        254
#define DEFAULT_ADMIN_USERNAME            "admin"
#define DEFAULT_ADMIN_PASSWORD            "admin"

// ---------------------------------------------------------------------------------------------------------------------
#endif // SERVER_CONFIGS_H