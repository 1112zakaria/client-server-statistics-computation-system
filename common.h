#ifndef COMMON_H
#define COMMON_H

#include <sys/msg.h>
#include <stdio.h>

#define DEBUG 0
#define MAX_TEXT 512
#define CLIENT_KEY 1238
#define SERVER_KEY 1239
#define INITIAL_CAPACITY 1
#define MICRO_SEC_IN_SEC 1000000
#define client_print(x) (                \
    {                                    \
        printf("client#%d: ", getpid()); \
        printf x;                        \
    })
#define server_print(x) (                \
    {                                    \
        printf("server#%d: ", getpid()); \
        printf x;                        \
    })
#define debug_print(x) (       \
    {                          \
        if (DEBUG)             \
        {                      \
            printf("DEBUG: "); \
            printf x;          \
        }                      \
    })
// Structures to be used
typedef enum Command_Type
{
    ERROR = -1,
    INSERT = 0,
    DELETE = 1,
    SUM = 2,
    AVERAGE = 3,
    MIN = 4,
    MEDIAN = 5,
    END = 6
} Command_Type;

struct client_data_t
{
    long int client_id;
    enum Command_Type command;
    int argument;

} client_data_t;

typedef enum message_errors {
    SUCCESS = 0,
    NO_CLIENT_FAIL = 1,
    NO_ELEMENTS = 2,
    MAX_ELEMENTS = 3
}message_errors_t;

struct server_data_t
{
    long int client_id;
    message_errors_t msg_response;
    int32_t value1;
    int32_t value2;
    int32_t num_values;
};




// Common prototypes
int create_msg_queue(key_t);
void send_msg();
void receive_msg();

#endif
