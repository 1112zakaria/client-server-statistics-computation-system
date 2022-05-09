#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/time.h>
#include "common.h"

// Please make it single-client for now...
static struct head_node_t *user_data;
static int capacity = INITIAL_CAPACITY;
static int size = 1;

/**
 * Design for calcuator:
 * 1. Create input and output msg queues
 * 2. Wait for data to enter input msg queue
 * 3. When input data is received, identify
 * the sender
 *  a. If the PID is new, add another row to
 *      the list of data
 *  b. Else if it exists, then append/prepend
 *      to the row's list
 *  c. Else if 'end', then delete the user's
 *      data
 *      i. If end
 * 4. Send a response message encapsulated
 * 
 * 
 * Questions:
 * - Maybe I could use a struct that defines
 *  command and format using enums? Client
 *  would feed data encapsulated this way
 *      struct input_data_t {
 *          long int user_id;
 *          Command_Type command;
 *          int has_argument;
 *          int argument;
 *      } 
 * - Should the message queues be blocking
 *  or non-blocking
 * Ideas:
 * - ASSUME that all input data is correctly
 *  formatted
 * - Use 2-D linked-list to store user data
 *      struct user_data_t {
 *          long int user_id;
 *          int value;
 *          user_data_t* next_value;
 *          user_data_t* next_user;
 *      }
 * - Or I could use a dynamically allocated
 *  2D array, where the first index of every
 *  row would be the user ID, but then I would
 *  have to manually:
 *      - shift when deleting
 *      - 
 * - Or I could not complicate things and stick
 *  to 1 user's data? (Suboptimal but most time-efficient
 *  one)
 * - Output struct could be:
 *      struct output_data_t {
 *          long int server_id;
 *          Command_Type command;
 *          int result;
 *      }
 * 
 * ISSUES:
 *  - Current clients are not being detected
 *      - user_data[0][0] changes from 0 to 1702063721
 *          when the 2nd input is added.
 *      - the find_client() function seems to cause
 *          change in 2D-array values?
 *      - What is the problem? A buffer overflow?
 *          Maybe my casual use of strings broke something?
 *      - SOLUTION: increase buffer size and clear string buffers?
 */

/**
 * @brief 
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[])
{
    run_server();
}

void run_server()
{

    int num_rows = 0; // this is the next available row
    int num_clients = 0;
    int client_queue_id;
    int server_queue_id;
    struct client_data_t client_msg;
    struct server_data_t server_msg;
    //struct my_msg_st test; // observation: this struct works
    int running = 1;
    int client_id = -1;
    char response[BUFSIZ];

    struct timeval start, end;
    float avg_time_ms = 0;
    float curr_time;
    int num_timestamps = 0;

    // Data format:
    //  Each row will follow [client_id, num_elements, ...]
    // When deleting the data, set the client_id to -1
    // Num rows will always be increasing and will break
    //  after 100 clients have used the server? (or make
    //  it shut down?)

    // Create message queues
    client_queue_id = create_msg_queue(CLIENT_KEY);
    server_queue_id = create_msg_queue(SERVER_KEY);

    initialize_data();

    while (running)
    {

        sleep(2);
        server_print(("Waiting for message...\n"));
        strncpy(server_msg.response, "", 1);
        if (msgrcv(client_queue_id, (void *)&client_msg, BUFSIZ,
                   0, 0) == -1)
        {
            fprintf(stderr, "msgrcv failed with error: %d\n%s\n",
                    errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Start time
        gettimeofday(&start, NULL);
        run_command(client_msg, &server_msg);
        gettimeofday(&end, NULL);
        // End time
        curr_time = (end.tv_sec * MICRO_SEC_IN_SEC + end.tv_usec) -
                    (start.tv_sec * MICRO_SEC_IN_SEC + start.tv_usec);
        num_timestamps += 1;
        // Add time to average
        if (avg_time_ms == 0)
        {
            avg_time_ms = curr_time;
        }
        else
        {
            avg_time_ms = avg_time_ms + (curr_time - avg_time_ms) / num_timestamps;
        }

        // Print computation time and average time
        server_print(("cmd execution time: %.2fms, avg execution time: %.2fms\n", curr_time, avg_time_ms));

        // Test response. Response works. Fails w/o string as part of msg
        server_msg.client_id = client_msg.client_id;
        server_msg.server_id = getpid();
        //strcpy(server_msg.response, "Hello there.");
        if (client_msg.command == END)
        {
            running = 0;
        }
        // Client is having problem transmitting message back
        debug_print(("%s\n", server_msg.response));
        send_msg(server_queue_id, &server_msg);
    }

    // Close the message queues
    server_print(("Shutting down server.\n"));
}

void send_msg(int msgid, struct server_data_t *data)
{
    debug_print(("Sending message\n"));
    if (msgsnd(msgid, (void *)data, BUFSIZ, 0) == -1)
    {
        fprintf(stderr, "msgsnd failed\n%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    debug_print(("Message sent.\n"));
}

void initialize_data()
{

    user_data = (struct head_node *)malloc(sizeof(struct head_node_t) * INITIAL_CAPACITY);
    user_data->client_id = DEFAULT_CLIENT_ID;
    user_data->average = 0;
    user_data->sum = 0;
    user_data->min = NULL;
    user_data->size = 0;
    user_data->next = NULL;
}

void run_command(struct client_data_t client, struct server_data_t *server)
{
    char *result_message = (char *)malloc(sizeof(char) * BUFSIZ);
    int *median = (int *)malloc(sizeof(int) * 3);
    strcpy(result_message, "");
    // Switch caase works. nvm doesnt work?
    /*
    Command_Type check = client->command;
    switch (check)
    {
    case INSERT:
        strcpy(server->response, "Insert!");
        break;
    case DELETE:
        strcpy(server->response, "Delete!");
        break;
    case SUM:
        strcpy(server->response, "Sum!");
        break;
    case AVERAGE:
        strcpy(server->response, "Average!");
        break;
    case MIN:
        strcpy(server->response, "Min!");
        break;
    case MEDIAN:
        strcpy(server->response, "Median!");
        break;
    case END:
        strcpy(server->response, "Shutting down");
        break;
    case ERROR:
        strcpy(server->response, "An unexpected error occurred");
        break;
    }
    */
    debug_print(("%d\n", client.command));
    if (client.command == INSERT)
    {
        //strcpy(result_message, "Insert!");
        sprintf(result_message, "%d has been inserted", client.argument);
        insert_value(client.argument);
    }
    else if (client.command == DELETE)
    {
        //strcpy(result_message, "Delete!");
        sprintf(result_message, "All %d occurrences have been deleted", client.argument);
        delete_value(client.argument);
    }
    else if (client.command == SUM)
    {
        //strcpy(result_message, "Sum!");

        sprintf(result_message, "The sum is %d", get_sum());
    }
    else if (client.command == AVERAGE)
    {
        //strcpy(result_message, "Average!");
        sprintf(result_message, "The average is %d", get_average());
    }
    else if (client.command == MIN)
    {
        //strcpy(server->response, "Min!");
        if (user_data->size > 0)
        {
            sprintf(result_message, "The minimum is %d", get_min());
        }
        else
        {
            sprintf(result_message, "There are no values");
        }
    }
    else if (client.command == MEDIAN)
    {
        //strcpy(server->response, "Median!");
        get_median(median);
        if (median[0] == 0)
        {
            sprintf(result_message, "There are no values");
        } else if (median[0] == 1) {
            sprintf(result_message, "The median is %d", median[1]);
        } else if (median[0] == 2) {
            sprintf(result_message, "The medians are %d and %d", median[1], median[2]);
        }
    }
    else if (client.command == END)
    {
        //strcpy(server->response, "Shutting down");
        sprintf(result_message, "Shutting down!");
    }
    else if (client.command == ERROR)
    {
        //strcpy(server->response, "An unexpected error occurred");
        sprintf(result_message, "An unexpected error occurred!");
    }

    strcpy(server->response, result_message);
}

void insert_value(int val) {
    struct value_node_t* curr = user_data->next;
    struct value_node_t* prev = NULL;

    // Create value node
    struct value_node_t* new_node = (struct value_node_t*)malloc(sizeof(struct value_node_t));
    new_node->value = val;
    new_node->next = NULL;

    // Increment size
    user_data->size += 1;

    // Keep iterating until: 
    //  - current node is greater than or equal to new node
    //  - current node is NULL
    // Update min if first node (prev = NULL)
    // Update max if last node (new_node->next = NULL)
    if (curr == NULL) {
        // If first element
        user_data->next = new_node;
        user_data->min = new_node;
        user_data->average = new_node->value;
        
    }

    while (curr.value >= new_node->value && curr != NULL) {
        prev = curr;
        curr = curr->next;
    }

}
