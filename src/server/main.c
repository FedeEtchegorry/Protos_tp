#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "args.h"
#include "serverConfigs.h"
#include "./core/selector.h"
#include "./client/pop3Server.h"
#include "./manager/managerServer.h"
#include "./logging/metrics.h"
#include "./logging/logger.h"

static bool done = false;

server_configuration clientServerConfig = {0};
server_metrics *clientMetrics = NULL;
server_logger *logger = NULL;

static void sigterm_handler(const int signal) {

    printf("Signal %d, cleaning up and exiting...\n", signal);
    done = true;
}

int main(const int argc, char** argv) {

    int clientServer = -1;
    int managerServer = -1;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    close(STDIN_FILENO);

    //------------------------- Registro el sigHandler para la terminación del programa --------------------------------

    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    //------------------------- Parsear argumentos ---------------------------------------------------------------------
    // Defined here to be used in finally
    struct pop3Args args;
    selector_status selectorStatus = SELECTOR_SUCCESS;
    char* errMsg = "?";
    //-------------------------------------

    parse_args(argc, argv, &args);

    if (args.maildir == NULL) {
        errMsg = "No maildir specified";
        goto finally;
    }
    const struct pop3Config pop3Config = {
        .maildir = args.maildir,
        .nusers = args.nusers,
        .users = args.users,
    };
    initPOP3Config(pop3Config);

    if(args.transformation_enabled) {
        setTransformationCommand(args.transformation_command);
    }

    clientServerConfig.ioReadBufferSize = DEFAULT_IO_BUFFER_SIZE;
    clientServerConfig.ioWriteBufferSize = DEFAULT_IO_BUFFER_SIZE;
    clientMetrics = serverMetricsCreate(HISTORIC_DATA_FILE, &clientServerConfig.ioReadBufferSize, &clientServerConfig.ioWriteBufferSize);

    //------------------------- CLIENT: Defino estructura para el socket para soportar IPv6 ----------------------------
    struct sockaddr_in6 addr = {0};

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(args.socks_port);

    if (inet_pton(AF_INET6, args.socks_addr, &addr.sin6_addr) != 1) {
        errMsg = "Invalid IPv6 address for Client Server";
        goto finally;
    }

    //-------------------------- MANAGER: Defino estructura para el socket para soportar IPv6  -------------------------

    struct sockaddr_in6 managerAddr = {0};

    managerAddr.sin6_family = AF_INET6;
    managerAddr.sin6_port = htons(args.mng_port);

    if (inet_pton(AF_INET6, args.mng_addr, &managerAddr.sin6_addr) != 1) {
        errMsg = "Invalid IPv6 address for Manager Server";
        goto finally;
    }

    //------------------------- CLIENT: Abro socket y consigo el fd ----------------------------------------------------

    clientServer = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if (clientServer < 0) {
        errMsg = "Unable to create socket";
        goto finally;
    }

    fprintf(stdout, "Listening on TCP port %d for Client Server\n", args.socks_port);

    setsockopt(clientServer, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    //------------------------- MANAGER: Abro socket y consigo el fd ---------------------------------------------------

    managerServer = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if (managerServer < 0) {
        errMsg = "Unable to create manager socket";
        goto finally;
    }

    fprintf(stdout, "Listening on TCP port %d for Manager Server\n", args.mng_port);

    setsockopt(managerServer, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    //------------------------- CLIENT: Bindeo el socket con la estructura creada --------------------------------------

    if (bind(clientServer, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        errMsg = "Unable to bind socket for Client Server";
        goto finally;
    }

    if (listen(clientServer, MAX_PENDING_CONNECTION_REQUESTS) < 0) {
        errMsg = "Unable to listen (client)";
        goto finally;
    }

    if (selector_fd_set_nio(clientServer) == -1) {
        errMsg = "Unable to set non-blocking mode for fd of Client Server";
        goto finally;
    }

    //------------------------- MANAGER: Bindeo el socket del MANAGER con la estructura creada -------------------------

    if (bind(managerServer, (struct sockaddr*)&managerAddr, sizeof(managerAddr)) < 0) {
        errMsg = "Unable to bind socket for Manager Server";
        goto finally;
    }

    if (listen(managerServer, MAX_PENDING_CONNECTION_REQUESTS) < 0) {
        errMsg = "Unable to listen (manager)";
        goto finally;
    }

    if (selector_fd_set_nio(managerServer) == -1) {
        errMsg = "Unable to set non-blocking mode for fd of Manager Server";
        goto finally;
    }

    //-------------------------- Inicializo el selector ----------------------------------------------------------------

    fd_selector selector = NULL;

    const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec = 10,
            .tv_nsec = 0,
        },
    };

    if (selector_init(&conf) != 0) {
        errMsg = "Initializing selector";
        goto finally;
    }

    selector = selector_new(SELECTOR_SIZE);

    if (selector == NULL) {
        errMsg = "Unable to create selector";
        goto finally;
    }

    #if LOG_DEFAULT_ENABLED == 1
        logger = serverLoggerCreate(&selector, LOG_DATA_FILE); // Ahora que tengo selector creo el logger
        serverLoggerRegister(logger, "STARTUP");
    #endif

    //----------------------------- CLIENT: Registro a mi socket pasivo para que acepte conexiones ---------------------

    const fd_handler clientPassiveSocket = {
        .handle_read = pop3PassiveAccept,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    selectorStatus = selector_register(selector, clientServer, &clientPassiveSocket, OP_READ, NULL);

    if (selectorStatus != SELECTOR_SUCCESS) {
        errMsg = "Registering Client Server fd";
        goto finally;
    }

    //----------------------------- MANAGER: Registro a mi socket pasivo para que acepte conexiones --------------------

    const fd_handler managerPassiveSocket = {
        .handle_read = manager_passive_accept,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    selectorStatus = selector_register(selector, managerServer, &managerPassiveSocket, OP_READ, NULL);

    if (selectorStatus != SELECTOR_SUCCESS) {
        errMsg = "Registering Manager Server fd";
        goto finally;
    }

    //---------------------------- Itero infinitamente haciendo select -------------------------------------------------

    while (!done) {

        selectorStatus = selector_select(selector);

        if (selectorStatus != SELECTOR_SUCCESS) {
            errMsg = "Serving";
            goto finally;
        }
    }

    errMsg = "Closing";

    //---------------------------- Terminación del programa ------------------------------------------------------------

    int ret = 0;

finally:

    if (selectorStatus != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", errMsg, selectorStatus == SELECTOR_IO ? strerror(errno) : selector_error(selectorStatus));
        ret = 2;
    } else {
        perror(errMsg);
        ret = 1;
    }

    if (selector != NULL) {
        selector_destroy(selector);
    }

    selector_close();

    if (clientServer >= 0) {
        close(clientServer);
    }

    if (managerServer >= 0) {
        close(managerServer);
    }

    serverMetricsRecordInFile(clientMetrics);
    serverMetricsFree(&clientMetrics);

    if (logger != NULL) {
        serverLoggerRegister(logger, "SHUTDOWN");
        serverLoggerTerminate(&logger);
    }

    return ret;
}