#ifndef PA1_STARTER_CODE_MAIN_H
#define PA1_STARTER_CODE_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include "ipc.h"
#include "common.h"
#include "logs.h"
#include "pa1.h"

#define MAX_PROCESS 11

typedef struct {
    int read_fd;
    int write_fd;
} PipeFd;

typedef struct {
    local_id current_id;
    PipeFd *pipes_arr[MAX_PROCESS][MAX_PROCESS];
} PipesArray;


void open_pipes();

void log_pipe(int num_event, int from, int to, int fd);

void pipe_work(local_id id);

void fork_processes();

void open_pipes();

void close_redundant_pipes(local_id id);

void wait_all_started(local_id id);

void wait_all_done(local_id id);

void close_self_pipes(local_id id);

void log_event(int log_type, local_id id);

void log_pipe(int log_type, int from, int to, int fd);

void parent_work();

void wait_children();

MessageHeader create_message_header(uint16_t len, int16_t type, timestamp_t time);

#endif
