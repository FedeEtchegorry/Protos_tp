#include "auth.h"

#include <string.h>

#include "parser.h"
#include "POP3Server.h"
#include "selector.h"

enum parserStates {
    START,
    U,
    US,
    USE,
    USER,
    USER_READ,
    P,
    PA,
    PAS,
    PASS,
    PASS_READ,
};

/*
static const struct parser_state_transition START_ST [] =  {
   {.when = 'F',        .dest = U,         .act1 = may_eq, },
   {.when = 'f',        .dest = 1,         .act1 = may_eq, },
   {.when = ANY,        .dest = NEQ,       .act1 = neq,},
};
static const struct parser_state_transition START_ST [] =  {
    {.when = 'F',        .dest = 1,         .action1 = may_eq, },
    {.when = 'f',        .dest = 1,         .action1 = may_eq, },
    {.when = ANY,        .dest = NEQ,       .action1 = neq,},
 };
static const struct parser_state_transition START_ST [] =  {
    {.when = 'F',        .dest = 1,         .action1 = may_eq, },
    {.when = 'f',        .dest = 1,         .action1 = may_eq, },
    {.when = ANY,        .dest = NEQ,       .action1 = neq,},
 };
static const struct parser_state_transition START_ST [] =  {
    {.when = 'F',        .dest = 1,         .action1 = may_eq, },
    {.when = 'f',        .dest = 1,         .action1 = may_eq, },
    {.when = ANY,        .dest = NEQ,       .action1 = neq,},
 };
static const struct parser_state_transition START_ST [] =  {
    {.when = 'F',        .dest = 1,         .action1 = may_eq, },
    {.when = 'f',        .dest = 1,         .action1 = may_eq, },
    {.when = ANY,        .dest = NEQ,       .action1 = neq,},
 };
static const struct parser_state_transition START_ST [] =  {
    {.when = 'F',        .dest = 1,         .action1 = may_eq, },
    {.when = 'f',        .dest = 1,         .action1 = may_eq, },
    {.when = ANY,        .dest = NEQ,       .action1 = neq,},
 };
static const struct parser_state_transition START_ST [] =  {
    {.when = 'F',        .dest = 1,         .action1 = may_eq, },
    {.when = 'f',        .dest = 1,         .action1 = may_eq, },
    {.when = ANY,        .dest = NEQ,       .action1 = neq,},
};
static const struct parser_state_transition START_ST [] =  {
    {.when = 'F',        .dest = 1,         .action1 = may_eq, },
    {.when = 'f',        .dest = 1,         .action1 = may_eq, },
    {.when = ANY,        .dest = NEQ,       .action1 = neq,},
};
*/
/*
static const struct parser_state_transition ** transitions = {
    1
};

static const size_t states_n = {};

static const struct parser_definition parserDefinition = {
    .start_state = START,
    .states = transitions,
    .states_n = states_n,
    .states_count = PASS_READ
};

void authReadInit(const unsigned state, struct selector_key * key) {
    clientData * data = ATTACHMENT(key);
    data->parsers.authParser = parser_init(parser_no_classes(), &parserDefinition);
}
*/
unsigned authRead(struct selector_key * key) {
    clientData* data = ATTACHMENT(key);

    size_t readLimit;
    uint8_t* readBuffer = buffer_write_ptr(&data->readBuffer, &readLimit);

    const ssize_t readCount = recv(key->fd, readBuffer, readLimit, 0);

    buffer_write_adv(&data->readBuffer, readCount);
    /*
    authParse(&data->parsers.authParser, &data->readBuffer);
    if (hasAuthReadEnded(&data->client.authParser)) {
        TAuthParser* authpdata = &data->client.authParser;
        TUserPrivilegeLevel upl;
        TUserStatus userStatus = validateUserAndPassword(authpdata, &upl);

        switch (userStatus) {
            case EUSER_OK:
                data->isAuth = true;
                break;
            case EUSER_WRONGUSERNAME:
                break;
            case EUSER_WRONGPASSWORD:
                break;
            default:
                break;
        }

        if (selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS || fillAuthAnswer(&data->client.authParser, &data->originBuffer)) {
            return ERROR;
        }
        return AUTH_WRITE;
    }
    return AUTH_READ;
    */
}

/*
unsigned authWrite(TSelectorKey* key) {
    clientData* data = ATTACHMENT(key);

    size_t writeLimit;    // how many bytes we want to send
    ssize_t writeCount;   // how many bytes where written
    uint8_t* writeBuffer; // buffer that stores the data to be sended

    writeBuffer = buffer_read_ptr(&data->originBuffer, &writeLimit);
    writeCount = send(key->fd, writeBuffer, writeLimit, MSG_NOSIGNAL);

    if (writeCount < 0)
        return ERROR;
    if (writeCount == 0)
        return ERROR;
    buffer_read_adv(&data->originBuffer, writeCount);

    if (buffer_can_read(&data->originBuffer)) {
        return AUTH_WRITE;
    }

    if (hasAuthReadErrors(&data->client.authParser) || data->client.authParser.verification == AUTH_ACCESS_DENIED || selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
        return ERROR;
    }

    return REQUEST_READ;
}
*/