#include "main.h"

int fd_events_log;
int fd_pipes_log;
int N;
//PipesArray *pipesArray;
PipeFd *pipesArray[11][11];

int main(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[1], "p") != 0) {
        return 0;
    }
  /*  while ((opt = getopt(argc, argv, "p:")) > 0) {
        if (opt == 'p') {
            N = (local_id) (atoi(optarg) + 1);
            return true;
        }
    }*/


    N = atoi(argv[2]) + 1;

    fd_pipes_log = open(pipes_log, O_APPEND | O_CREAT | O_TRUNC, 0777);
    fd_events_log = open(events_log, O_APPEND | O_CREAT | O_TRUNC, 0777);

    // open pipes
    open_pipes();

    fork_processes();

    parent_work();

    close(fd_pipes_log);
    close(fd_events_log);
    return 0;
}

void open_pipes() {
//    *pipesArray = (PipesArray *) malloc(N * (N - 1) * sizeof(PipesArray));

    int fds[2];

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i != j) {
                PipeFd *pipeFd = (PipeFd *) malloc(sizeof(PipeFd));
                PipeFd *pipeFd1 = (PipeFd *) malloc(sizeof(PipeFd));
                pipesArray[i][j] = pipeFd;
                pipesArray[j][i] = pipeFd1;

                pipe(fds);

                pipesArray[i][j]->write_fd = fds[1];
                log_pipe(0, i, j, pipesArray[i][j]->write_fd);

                pipesArray[j][i]->read_fd = fds[0];
                log_pipe(0, j, i, pipesArray[j][i]->read_fd);
            }
        }
    }
}

void fork_processes() {

    pid_t all_pids[N];

    all_pids[0] = getpid();
    // fork processes
    for (local_id i = 1; i < N; i++) {
        all_pids[i] = fork();
        if (all_pids[i] == 0) {
            pipe_work(i);
            exit(0);
        } else if (all_pids[i] == -1) {
            //error
        }
    }
}

void pipe_work(local_id id) {
    close_redundant_pipes(id);
    wait_all_started(id);
    wait_all_done(id);
    close_self_pipes(id);
}

void close_redundant_pipes(local_id id) {
    PipeFd *pipe_fd;
    for (local_id i = 0; i < N; i++) {
        if (i == id) continue;
        for (local_id j = 0; j < N; j++) {
            if (i != j) {
                pipe_fd = pipesArray[i][j];
                log_pipe(1, i, j, pipe_fd->write_fd);
                close(pipe_fd->write_fd);
                log_pipe(1, i, j, pipe_fd->read_fd);
                close(pipe_fd->read_fd);
            }
        }
    }
}

void wait_all_started(local_id id) {
    log_event(0, id);

    Message message;
    int len = sprintf(message.s_payload, log_started_fmt, id, getpid(), getppid());
    message.s_header = create_message_header(len, STARTED, time(NULL));

    if (send_multicast(&id, &message) != 0) {
        exit(1);
    }

    Message receive_message;
    receive_any(&id, &receive_message);
    log_event(1, id);
}

void wait_all_done(local_id id) {
    log_event(4, id);

    Message message;
    int len = sprintf(message.s_payload, log_started_fmt, id, getpid(), getppid());
    message.s_header = create_message_header(len, DONE, time(NULL));

    if (send_multicast(&id, &message) != 0) {
        exit(1);
    }

    Message receive_message;
    receive_any(&id, &receive_message);
    log_event(3, id);
}

void close_self_pipes(local_id id) {
    PipeFd *curr;
    for (local_id i = 0; i < N; i++) {
        if (i != id) {
            curr = pipesArray[id][i];
            log_pipe(1, id, i, curr->write_fd);
            close(curr->write_fd);
            log_pipe(1, id, i, curr->read_fd);
            close(curr->read_fd);
        }
    }
}

int send_multicast(void *self, const Message *msg) {
    local_id id = *(local_id *) self;
    for (local_id i = 0; i < N; i++) {
        if (i != id)
            if (send(self, i, msg) != 0) {
                return 1;
            }
    }
    return 0;
}

int send(void *self, local_id dst, const Message *msg) {
    local_id id = *(local_id *) self;
    int write_fd = pipesArray[id][dst]->write_fd;
    if (write(write_fd, msg, sizeof(MessageHeader) + msg->s_header.s_payload_len) > 0) {
        return 0;
    } else {
        return 1;
    }
}

int receive_any(void *self, Message *msg) {
    local_id id = *(local_id *) self;
    for (local_id i = 1; i < N; i++)
        if (i != id)
            if (receive(&id, i, msg) != 0)
                return 1;
    return 0;
}

int receive(void *self, local_id from, Message *msg) {
    local_id id = *(local_id *) self;
    PipeFd *pipe_fd = pipesArray[id][from];
    MessageHeader messageHeader;

    if (read(pipe_fd->read_fd, &messageHeader, sizeof(MessageHeader)) == -1) {
        return 1;
    }
    int len = messageHeader.s_payload_len;
    char buf[len];
    if (read(pipe_fd->read_fd, buf, len) == -1) {
        return 1;
    }
    msg->s_header = messageHeader;
    strcpy(msg->s_payload, buf);
    return 0;
}

void parent_work() {
    Message msg;
    local_id id = 0;
    close_redundant_pipes(id);
    receive_any(&id, &msg);
    log_event(1, id);
    receive_any(&id, &msg); //TODO как понять что parent получил все ответы
    log_event(3, id);
    wait_children();
    close_self_pipes(id);
}

void wait_children() {
    while (wait(NULL) > 0) {
    }
}

void log_pipe(int log_type, int from, int to, int fd) {
    char buf[100];

    switch (log_type) {
        case 0:
            sprintf(buf, pipe_opened_log, from, to, fd);
            break;
        case 1:
            sprintf(buf, pipe_closed_log, from, to, fd);
            break;
    }
    write(fd_pipes_log, buf, strlen(buf));
}

void log_event(int log_type, local_id id) {
    char buf[100];

    switch (log_type) {
        case 0:
            sprintf(buf, log_started_fmt, id, getpid(), getppid());
            break;
        case 1:
            sprintf(buf, log_received_all_started_fmt, id);
            break;
        case 4:
            sprintf(buf, log_done_fmt, id);
            break;
        case 3:
            sprintf(buf, log_received_all_done_fmt, id);
    }

    printf(buf, 0);
    write(fd_events_log, buf, strlen(buf));
}

MessageHeader create_message_header(uint16_t len, int16_t type, timestamp_t time) {
    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_payload_len = len;
    header.s_type = type;
    header.s_local_time = time;
    return header;
}
