#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/msg.h> 
#include <sys/time.h>
#include "common.h"

static int user_data[100][100]; // 2D user data list
static int num_rows = 0;        // this is the next available row
static int num_clients = 0;



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
 * @brief Removes a client
 * by marking their array as
 * unowned
 * 
 * @param index 
 */
static message_errors_t remove_client(int index)
{
    
    int *elements = user_data[index];
    if (elements[0] == -1) {
        return NO_CLIENT_FAIL; 
    }
    elements[0] = -1;
    elements[1] = 0;

    return 0;
}

/**
 * @brief Inserts value into
 * user's data array
 * 
 * @param index 
 * @param value 
 */
static message_errors_t insert_value(int index, int value)
{
    int* elements = user_data[index];
    if (elements[1] + 1 > 99) {
        return MAX_ELEMENTS;
    }
    elements[elements[1] + 2] = value;
    elements[1] += 1;
    return 0;
}

/**
 * @brief Deletes all occurrences
 * of value from user's data array
 * 
 * @param index 
 * @param value 
 */
static message_errors_t delete_value(int index, int value)
{
    int* elements = user_data[index];
    if (elements[1] < 1) {
        return NO_ELEMENTS;
    }
    for (int i = 2; i < elements[1] + 2; i++)
    {
        if (elements[i] == value)
        {
            for (int j = i; j < elements[1] + 2 - 1; j++)
            {
                elements[j] = elements[j + 1];
            }
            i -= 1;
            elements[1]--;
        }
    }
    return 0;
}

/**
 * @brief Get the sum of user's data array
 * 
 * @param index 
 * @return int 
 */
static message_errors_t get_sum(int index, int32_t *sum)
{
    int* elements = user_data[index];
    int count = elements[1];
    if (count == 0){
        return NO_ELEMENTS;
    }
    *sum = 0;
    for (int i = 2; i < count + 2; i++)
    {
        *sum += elements[i];
    }
    return 0;
}

/**
 * @brief Get the average of user's data array
 * 
 * @param index 
 * @return int 
 */
static message_errors_t get_average(int index, int32_t * avg)
{
    message_errors_t status;
    int* elements = user_data[index];
    if (elements[1] < 1){
        return NO_ELEMENTS;
    }
    
    int sum; 
    status = get_sum(index, &sum);
    if(status != SUCCESS){
        return status;
    }

    *avg = sum / elements[1];
    return SUCCESS;
}

/**
 * @brief Get the median of user's data array
 * 
 * @param index 
 * @return int* 
 */
static message_errors_t get_median(int index, int32_t *value1, int32_t *value2, int32_t *count)
{
    // sort then get middle
    // use insertion sort since it's dead simple
    // index 0 == 1 means 1 median, 1 means 2 medians
    int* elements = user_data[index];
    if(elements[1] < 1){
        return NO_ELEMENTS;
    }
    
    for (int i = 3; i < elements[1] + 2; i++)
    {
        int j = i;
        while (j > 2 && elements[j] < elements[j - 1])
        {
            int temp = elements[j];
            elements[j] = elements[j - 1];
            elements[j - 1] = temp;
            j -= 1;
        }
    }

    int len = elements[1];
    int midpoint = (int)len/2;
    if (elements[1] % 2 != 0)
    {
        *count = 1;
        *value1 = elements[2 + midpoint];
    }
    else
    {
        *count = 2;
        *value1 = elements[2 + midpoint];
        *value2 = elements[2 + midpoint - 1];
    }
    return SUCCESS;
}

/**
 * @brief Get the min element from user array
 * 
 * @param index 
 * @return int 
 */
static message_errors_t get_min(int index, int32_t *min)
{
    int* elements = user_data[index];
    if(elements[1] < 1){
        return NO_ELEMENTS;
    }
    *min = elements[2];
    for (int i = 2; i < elements[1] + 2; i++)
    {
        if (elements[i] < *min)
        {
            *min = elements[i];
        }
    }
    return SUCCESS;
}

/**
 * @brief Sets all array fields
 * as unoccupied
 * 
 */
void initialize_data()
{
    for (int i = 0; i < 100; i++)
    {
        user_data[i][0] = -1;
        user_data[i][1] = 0;
    }
    debug_print(("Data arrays initialized\n"));
}

/**
 * @brief Finds client index.
 * Returns -1 if not found.
 * 
 * @param client_id 
 * @return int 
 */
int find_client(int client_id)
{

    int index = -1;
    int* row;
    for (int i = 0; i < 100; i++)
    {
        debug_print(("client @ i=%d: %d\n", i, user_data[i][0]));
        row = &user_data[i][0];
        if (row[0] == client_id)
        {
            index = i;
            break;
        }
    }
    debug_print(("client#%d index: %d\n", client_id, index));
    return index;
}

/**
 * @brief Send message
 * 
 * @param msgid 
 * @param data 
 */
void send_msg(int msgid, struct server_data_t *data)
{
    debug_print(("Sending message\n"));
    if (msgsnd(msgid, (void *)data, sizeof(struct server_data_t) - sizeof(long), 0) == -1)
    {
        fprintf(stderr, "msgsnd failed\n%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    debug_print(("Message sent.\n"));
}

/**
 * @brief Adds client to the
 * data array
 * 
 * @param id 
 * @param num_clients 
 * @param num_rows 
 */
static void add_client(int id)
{

    user_data[num_rows][0] = id;
    user_data[num_rows][1] = 0;
    debug_print(("Added client#%d at index %d\n", id, num_rows));
    num_rows += 1;
    num_clients += 1;
    
    server_print(("Added client#%d\n", id));
}

/**
 * @brief Performs one of the 
 * actions depending on the 
 * client_data_t data struct
 * 
 * @param data 
 * @param index 
 * @param num_clients 
 * @return char* 
 */
static message_errors_t perform_action(struct client_data_t data, 
                                       int index, struct server_data_t *response)
{
    //char *result_message = (char *)malloc(sizeof(char) * BUFSIZ);
    //strcpy(result_message, "");
    //memset(result_message, 0, BUFSIZ);
    message_errors_t status = 0;

    if (data.command == END)
    {
        status = remove_client(index);
        num_clients -= 1;
        //sprintf(result_message, "client#%d's session has ended", (int)data.client_id);
    }
    else if (data.command == INSERT)
    {
        status = insert_value(index, data.argument);
        //sprintf(result_message, "%d has been added to client#%d's session data",
        //        data.argument, (int)data.client_id);
    }
    else if (data.command == DELETE)
    {
        status = delete_value(index, data.argument);
    }
    else if (data.command == SUM)
    {
         int32_t sum;
         status = get_sum(index, &sum);
         response->value1 = sum;
    }
    else if (data.command == AVERAGE)
    {
        int32_t avg;
        status = get_average(index, &avg);
        response->value1 = avg;        
    
    }
    else if (data.command == MIN)
    {

        int32_t min;
        status = get_min(index, &min);
        response->value1 = min;
    }
    else if (data.command == MEDIAN)
    {

        int32_t value1, value2, num_values;
        status = get_median(index, &value1, &value2, &num_values); // median[0] == 1 -> one median, else two medians
        response->value1 = value1;
        response->value2 = value2;
        response->num_values = num_values;
    }

    return status;
}


/**
 * @brief Run server
 * 
 */
message_errors_t run_server()
{
    
 
    int client_queue_id;
    int server_queue_id;
    struct client_data_t client_msg;
    struct server_data_t server_msg;
    //struct my_msg_st test; // observation: this struct works
    int running = 1;
    int client_id = -1;
    char response[BUFSIZ];

    struct timeval start, end;
    float curr_time = 0;
    float avg_time = 0;
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

    initialize_data();  // init data

    while (running)
    {

        // Wait for data to enter client queue
        // How do I make client wait
        // It appears to read the data but then
        //  crashes w/ errno 22 invalid argument
        sleep(2);
        debug_print(("num clients: %d, data[0][0]: %d\n", num_clients, user_data[0][0]));
        server_print(("Waiting for message...\n"));
        //strncpy(response, " ", 1);
        memset(response, 0, BUFSIZ);
        if (msgrcv(client_queue_id, (void *)&client_msg, sizeof(client_msg) -sizeof(long),
                   0, 0) == -1)
        {
            fprintf(stderr, "msgrcv failed with error: %d\n%s\n",
                    errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        debug_print(("client id: %d cmd: %d arg: %d\n",
                     (int)client_msg.client_id, client_msg.command, client_msg.argument));
        client_id = find_client((int)client_msg.client_id);


        if (client_id == -1)
        {
            // If client id is new, then add row to user_data
            //  and command is not END
            
            add_client(client_msg.client_id);
            client_id = find_client(client_msg.client_id);
        }
        server_msg.client_id = client_msg.client_id;
        server_msg.msg_response = SUCCESS;
        //server_msg.server_id = getpid();

        gettimeofday(&start, NULL);
#if 1
        // FIXME:
        
        message_errors_t status = perform_action(client_msg, client_id, &server_msg);
        server_msg.msg_response = status;

        gettimeofday(&end, NULL);

        curr_time = (end.tv_sec * MICRO_SEC_IN_SEC + end.tv_usec) -
            (start.tv_sec * MICRO_SEC_IN_SEC + start.tv_usec);
        num_timestamps += 1;
        if (avg_time == 0) {
            avg_time = curr_time;
        } else {
            avg_time = avg_time + (curr_time - avg_time)/num_timestamps;
        }
        
        server_print(("exec time: %.2fus, avg exec time: %.2fus, #cmds: %d\n", curr_time, avg_time, num_timestamps));
        // Else if client id exists and command is END,
        //  "delete" user
        //debug_print(("strlen of response: %lu\n", strlen(server_msg.response)));
        send_msg(server_queue_id, &server_msg);
        server_print(("%s\n", response));
        // If num clients == 0, shutdown server
        if (num_clients == 0)
        {
            running = 0;
        }
        // Else if client id exists
#endif
    }

#if 1
    // Close the message queues
    server_print(("Shutting down server.\n"));
    if (msgctl(client_queue_id, IPC_RMID, 0) == -1)
    {
        fprintf(stderr, "msgctl(IPC_RMID) 1 failed\n");
        exit(EXIT_FAILURE);
    }
    if (msgctl(server_queue_id, IPC_RMID, 0) == -1)
    {
        fprintf(stderr, "msgctl(IPC_RMID)2 failed\n");
        exit(EXIT_FAILURE);
    }
#endif
    debug_print(("exit success"));
    exit(EXIT_SUCCESS);
}

/**
 * @brief 
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[])
{
    exit(((int)run_server()));
}

