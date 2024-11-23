#include "managerServer.h"


#include <stdlib.h>
#include <string.h>

#include "selector.h"

#include <sys/socket.h>

#include "auth.h"
#include "buffer.h"
#include "greetings.h"
#include "pop3Parser.h"
#include "stm.h"
#include "transaction.h"
#include "managerParser.h"
#include "serverUtils.h"
#include <stdio.h>

#define BUFFER_SIZE 8192

#define SUCCESS_MSG "+OK "
#define ERROR_MSG "-ERR "

//-------------------------------------Array de estados para la stm------------------------------------------
static const struct state_definition stateHandlers[] = {
    {
        .state = GREETINGS,
        .on_arrival = greetingOnArrivalForManager,
        .on_write_ready = writeOnReady
    },
    {
        .state = AUTHORIZATION,
        .on_arrival = authOnArrival,
        .on_read_ready = readOnReady,
        .on_write_ready = writeOnReady,
    },
    {
        .state = TRANSACTION,
        .on_arrival = transactionOnArrivalForManager,
        .on_read_ready = readOnReady,
        .on_write_ready = writeOnReady,
    },
    {
        .state = UPDATE,
        .on_arrival = NULL,
        .on_write_ready = writeOnReady,
    },
    {
        .state = DONE,
    },
    {
        .state = ERROR
    }
};


//-----------------------------------Struct to storage relevant info for the manager in selectorKey---------------------
managerData * newManagerData(const struct sockaddr_storage managerAddress) {

  fprintf(stdout, "ENTRANDO FUNCION: %s(?)\n", __func__);
  fprintf(stdout, "LOCANDO: %ld BYTES\n", sizeof(struct managerData));
  managerData * managerData = calloc(1, sizeof(struct managerData));
  fprintf(stdout, "LINEA: %d\n", __LINE__);

  managerData->manager_data.stateMachine.initial = GREETINGS;
  managerData->manager_data.stateMachine.max_state = ERROR;
  managerData->manager_data.stateMachine.states = stateHandlers;
  fprintf(stdout, "LINEA: %d\n", __LINE__);
  stm_init(&managerData->manager_data.stateMachine);
  fprintf(stdout, "LINEA: %d\n", __LINE__);

  uint8_t* readBuffer = malloc(BUFFER_SIZE);
  uint8_t* writeBuffer = malloc(BUFFER_SIZE);
  fprintf(stdout, "LINEA: %d\n", __LINE__);
  buffer_init(&managerData->manager_data.readBuffer, BUFFER_SIZE, readBuffer);
  buffer_init(&managerData->manager_data.writeBuffer, BUFFER_SIZE, writeBuffer);
  const methodsMap * map = getManagerMethods();
  fprintf(stdout, "LINEA: %d\n", __LINE__);
  parserInit(&managerData->manager_data.parser, map);
  fprintf(stdout, "LINEA: %d\n", __LINE__);
  managerData->manager_data.currentUsername = NULL;
  managerData->manager_data.isAuth = false;
  managerData->manager_data.closed = false;

  managerData->manager_data.sockaddrStorage = managerAddress;

  return managerData;
}

//------------------------------Passive Socket--------------------------------------------------------
void manager_passive_accept(struct selector_key* key) {

  fprintf(stdout, "Entrando funcion\n");
  struct sockaddr_storage manager_addr;
  socklen_t manager_addr_len = sizeof(manager_addr);

  const int manager = accept(key->fd, (struct sockaddr*)&manager_addr, &manager_addr_len);

  fprintf(stdout, "Aceptando\n");

  if (manager == -1) {
    goto fail;
  }

  fprintf(stdout, "Aceptando 2\n");

  if (selector_fd_set_nio(manager) == -1) {
    goto fail;
  }

  fprintf(stdout, "Aceptando 3\n");

  managerData * managerData = newManagerData(manager_addr);

  fprintf(stdout, "Aceptando 4\n");
  if (SELECTOR_SUCCESS != selector_register(key->s, manager, getHandler(), OP_WRITE, managerData)) {
    goto fail;
  }
  return;

fail:
  if (manager != -1) {
    close(manager);
  }
}
