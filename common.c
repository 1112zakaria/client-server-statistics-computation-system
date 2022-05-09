#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/msg.h>
#include "common.h"


/**
 * @brief Create a msg queue
 * 
 * @param key 
 * @return int 
 */
int create_msg_queue(key_t key) {
    int msgid = msgget((key_t)key, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        fprintf(stderr, "msgget failed with error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    return msgid;
}

