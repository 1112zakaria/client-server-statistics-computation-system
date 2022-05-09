#ifndef COMMON_H
#define COMMON_H

#include <sys/msg.h>
#include <stdio.h>

#define DEBUG 1
#define MAX_TEXT 512
#define CLIENT_KEY 1234
#define SERVER_KEY 1235
#define INITIAL_CAPACITY 1
#define DEFAULT_CLIENT_ID 4321
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
    char word[MAX_TEXT]; // left as a placeholder
    enum Command_Type command;
    int argument;

} client_data_t;

struct server_data_t
{
    long int client_id;
    char response[MAX_TEXT];
    int server_id;
    //enum Command_Type command;
    //int result_len;
    //int result[2];
};

struct head_node_t {
    int client_id;
    float average;
    struct value_node_t* min;
    int sum;
    int size;
    struct value_node* next;
};

struct value_node_t {
    int value;
    struct value_node_t* next;
};

// User prototypes
void run_client(void);
void parse_input(char *, Command_Type *, int *);
Command_Type get_command(char *, int);

// Calculator prototypes
void run_server(void);
void initialize_data(void);
void run_command(struct client_data_t, struct server_data_t*);

int find_client(int);
void add_client(int, int*, int*);
char* perform_action(struct client_data_t, int, int*);
void remove_client(int);
void insert_value(int, int);
void delete_value(int, int);
int get_min(int);
int* get_median(int);
int get_sum(int);
int get_average(int);

// Common prototypes
int create_msg_queue(key_t);
void send_msg();
void receive_msg();

#endif
