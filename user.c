#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/msg.h>
#include "common.h"

// User prototypes
void run_client(void);
void parse_input(char *, Command_Type *, int *);
Command_Type get_command(char *, int);
void print_response(struct server_data_t, struct client_data_t);

/**
 * Design for user:
 * 1. Connect to input and output msg queues
 * 2. Loop until 'end' is input
 * 3. Parse input and validate format
 *  a. if valid, send to message queue, encapsulated
 *      in struct format
 *  b. else, inform the user of invalid input
 *      Valid formats:
 *          - end
 *          - delete N
 *          - sum
 *          - average
 *          - min
 *          - median
 * 4. Read the output
 * 5. End the program
 * 
 * Questions:
 * - How will I parse commands?
 * - Should the client handle input format
 *  or the server?
 *  - I will give the client this functionality
 * - Should I handle multiple users?
 *  - ENGINEERING ASSUMPTION: YES. Use long int msg_type
 *      w/ identifier being the PID. Each PID will have
 *      its own app data
 * 
 */

/**
 * @brief 
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char* argv[]) {
    run_client();
}

/**
 * @brief Run the client.
 * 
 */
void run_client() {
    int client_queue_id;
    int server_queue_id;
    int running = 1;
    char buffer[BUFSIZ];
    Command_Type input_command;
    int input_argument;
    struct client_data_t client_msg;
    struct server_data_t server_msg;
    //struct my_msg_st test;

    // Get message queues
    client_queue_id = create_msg_queue(CLIENT_KEY);
    server_queue_id = create_msg_queue(SERVER_KEY);

    while (running) {
        // Loop until 'end' is input
        printf("Enter command ('end' to quit session): ");
        fgets(buffer, BUFSIZ, stdin);
        // Parse input and validate format
        parse_input(buffer, &input_command, &input_argument);
        debug_print(("Enum value: %d\n", input_command));
        
        if (input_command != ERROR) {
            // If valid, encapsulate and send to
            //  message queue
            client_msg.client_id = getpid();
            client_msg.command = input_command;
            client_msg.argument = input_argument;
            debug_print(("arg: %d, cmd: %d\n", client_msg.argument, client_msg.command));
            //client_msg.command = SUM;
            //client_msg.argument = 0;
            //test.my_msg_type = getpid();
            //strcpy(test.some_text, "burger king");
            debug_print(("sizeof data: %d\n", (int)sizeof(client_msg)));

            // Question: how do i set to wait if full?
            debug_print(("Wating for server...\n"));            
            if (msgsnd(client_queue_id, (void *)&client_msg, sizeof(client_msg) - sizeof(long), 0) == -1)
            {
                fprintf(stderr, "msgsnd failed\n%s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            // How do I make client wait?
            if (msgrcv(server_queue_id, (void *)&server_msg, sizeof(struct server_data_t) -sizeof(long),
                   getpid(), 0) == -1)
            {
                fprintf(stderr, "msgrcv failed with error: %d\n", errno);
                exit(EXIT_FAILURE);
            }
            
            print_response(server_msg, client_msg);

            if (input_command == END) {
                running = 0;
            }

        } else {
            // Else, display error
            client_print(("Invalid input\n"));
        }

    }

    client_print(("Ending session.\n"));


}

void receive_msg(int msgid, struct server_data_t data) {

}

/**
 * @brief Parses user input. 
 * Sets the command type and
 * argument
 * 
 * @param input 
 * @param command 
 * @param arg 
 */
void parse_input(char* input, Command_Type* command, int* arg) {
    char command_word[BUFSIZ];
    char concat[BUFSIZ] = "";
    char string_arg[BUFSIZ] = "";
    Command_Type selected_command;
    int is_valid = 1;
    int has_argument = 0;

    sscanf(input, "%s %[0-9]s?", command_word, string_arg);
    *arg = atoi(string_arg);
    debug_print(("command word: %s\narg: %d\n", command_word, *arg));
    // Observation: Parser will get key info but a 2nd check
    //  verifying that the 'command_word arg' == 'input' or
    //  'command_word' == 'input'
    strcat(concat, command_word);
    if (strcmp(string_arg, "") != 0) {
        // Has argument
        has_argument = 1;
        strcat(concat, " ");
        strcat(concat, string_arg);
    }

    // Problem: there is an invisible character somewhere in input
    // Hacky solution: set last character as NUL char?
    int inputlen = strlen(input);
    input[inputlen-1] = '\0';

    debug_print(("Concat string (len %d): %s\nInput string (len %d): %s\n", 
        (int)strlen(concat), concat, (int)strlen(input), input));

    if (strcmp(concat, input) == 0) {
        // Input matches parsed data (sscanf and string are kinda weirdchamp)
        debug_print(("INPUT MATCHES\n"));
        // Get command
        selected_command = get_command(command_word, has_argument);
        if (selected_command == ERROR) {
            is_valid = 0;
        }
    } else {
        is_valid = 0;
        debug_print(("DIFFERENT INPUT: %d\n", strcmp(concat, input)));
    }

    //return (is_valid) ? selected_command : ERROR;
    *command = (is_valid) ? selected_command : ERROR;
    
}

/**
 * @brief Get the command type,
 * based on whether input str
 * has arg or not
 * 
 * @param command_string 
 * @param has_arg 
 * @return Command_Type 
 */
Command_Type get_command(char* command_string, int has_arg) {
    char* valid_commands[] = {
        "insert",
        "delete",
        "sum",
        "average",
        "min",
        "median",
        "end"
    };
    int num_commands = 7;
    Command_Type command = ERROR;

    // lower input word
    char* p = command_string;
    for ( ; *p; ++p) *p = tolower(*p);

    for (int i=0; i<num_commands; i++) {
        if (strcmp(command_string, valid_commands[i]) == 0) {
            command = (Command_Type)i;
        }
    }

    if (!has_arg && (command == INSERT || command == DELETE)) {
        command = ERROR;
    }

    return command;
}

/**
 * @brief Prints response based on
 * server status output and command
 * executed
 * 
 * @param server 
 * @param client 
 */
void print_response(struct server_data_t server, struct client_data_t client) {
    char* err_msgs[3] = {
        "Client does not exist",
        "No elements stored",
        "Max elements stored"
    };

    if (server.msg_response != SUCCESS) {
        server_print(("An error occured: code %d\n%s\n", 
            server.msg_response, err_msgs[server.msg_response-1]));
        return;
    }
    server_print(("Success.\n"));

    switch (client.command) {
        case INSERT:
            server_print(("Inserted %d\n", client.argument));
            break;
        case DELETE:
            server_print(("Deleted all occurrences of %d\n", client.argument));
            break;
        case SUM:
            server_print(("Sum is %d\n", server.value1));
            break;
        case AVERAGE:
            server_print(("Average is %d\n", server.value1));
            break;
        case MIN:
            server_print(("Min is %d\n", server.value1));
            break;
        case MEDIAN:
            switch(server.num_values) {
                case 1:
                    server_print(("Median is %d\n", server.value1));
                    break;
                case 2:
                    server_print(("Medians are %d and %d\n", 
                        server.value1, server.value2));
                    break;
            }
            break;
        case END:
            server_print(("Client data has been removed\n"));
            break;
        case ERROR:
            server_print(("An unexpected error has occurred\n"));
            break;
    }
}