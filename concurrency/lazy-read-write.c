#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>

#define MAX_REQUESTS 100
#define MAX_FILES 100

#define YELLOW "\033[33m"
#define PINK "\033[95m"
#define GREEN "\033[32m"
#define RED "\033[31m"
#define WHITE "\033[37m"
#define RESET "\033[0m"

typedef enum {
    READ,
    WRITE,
    DELETE
} Operation;

typedef struct {
    int user_id;
    int file_id;
    Operation op;
    int arrival_time;
    bool is_cancelled;
    bool is_completed;
    pthread_t thread;
} Request;

typedef struct {
    int readers;
    int writers;
    bool is_deleted;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} File;

int read_time, write_time, delete_time;
int num_files, max_concurrent, patience_time;
Request requests[MAX_REQUESTS];
File files[MAX_FILES];
int num_requests = 0;
int current_time = 0;
pthread_mutex_t time_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t time_cond = PTHREAD_COND_INITIALIZER;

int get_operation_time(Operation op) {
    switch (op) {
        case READ: return read_time;
        case WRITE: return write_time;
        case DELETE: return delete_time;
        default: return 0;
    }
}

const char* get_operation_name(Operation op) {
    switch (op) {
        case READ: return "READ";
        case WRITE: return "WRITE";
        case DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}

void print_message(const char* color, const char* format, ...) {
    pthread_mutex_lock(&time_mutex);
    va_list args;
    va_start(args, format);
    printf("%s", color);
    vprintf(format, args);
    printf("%s\n", RESET);
    va_end(args);
    pthread_mutex_unlock(&time_mutex);
}

void* process_request(void* arg) {
    Request* req = (Request*)arg;
    int wait_time = req->arrival_time + 1;
    
    pthread_mutex_lock(&time_mutex);
    while (current_time < wait_time) {
        pthread_cond_wait(&time_cond, &time_mutex);
    }
    pthread_mutex_unlock(&time_mutex);

    if (req->is_cancelled) {
        return NULL;
    }

    File* file = &files[req->file_id - 1];
    bool can_process = false;

    pthread_mutex_lock(&file->mutex);
    
    if (file->is_deleted) {
        pthread_mutex_unlock(&file->mutex);
        print_message(WHITE, "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested.",
                     req->user_id, current_time);
        return NULL;
    }

    print_message(PINK, "LAZY has taken up the request of User %d at %d seconds",
                 req->user_id, current_time);

    switch (req->op) {
        case READ:
            while (file->writers > 0) {
                pthread_cond_wait(&file->cond, &file->mutex);
            }
            file->readers++;
            can_process = true;
            break;

        case WRITE:
            while (file->readers > 0 || file->writers > 0) {
                pthread_cond_wait(&file->cond, &file->mutex);
            }
            file->writers++;
            can_process = true;
            break;

        case DELETE:
            while (file->readers > 0 || file->writers > 0) {
                pthread_cond_wait(&file->cond, &file->mutex);
            }
            file->is_deleted = true;
            can_process = true;
            break;
    }

    pthread_mutex_unlock(&file->mutex);

    if (can_process) {
        sleep(get_operation_time(req->op));
        
        pthread_mutex_lock(&file->mutex);
        if (req->op == READ) {
            file->readers--;
        } else if (req->op == WRITE) {
            file->writers--;
        }
        pthread_cond_broadcast(&file->cond);
        pthread_mutex_unlock(&file->mutex);

        pthread_mutex_lock(&time_mutex);
        current_time += get_operation_time(req->op);
        req->is_completed = true;
        print_message(GREEN, "The request for User %d was completed at %d seconds",
                     req->user_id, current_time);
        pthread_cond_broadcast(&time_cond);
        pthread_mutex_unlock(&time_mutex);
    }

    return NULL;
}

void* time_keeper(void* arg) {
    while (true) {
        sleep(1);
        pthread_mutex_lock(&time_mutex);
        current_time++;
        
        for (int i = 0; i < num_requests; i++) {
            if (!requests[i].is_completed && !requests[i].is_cancelled && 
                current_time >= requests[i].arrival_time + patience_time) {
                requests[i].is_cancelled = true;
                print_message(RED, "User %d canceled the request due to no response at %d seconds",
                            requests[i].user_id, current_time);
            }
        }
        
        bool all_done = true;
        for (int i = 0; i < num_requests; i++) {
            if (!requests[i].is_completed && !requests[i].is_cancelled) {
                all_done = false;
                break;
            }
        }
        
        pthread_cond_broadcast(&time_cond);
        pthread_mutex_unlock(&time_mutex);
        
        if (all_done) {
            print_message(WHITE, "LAZY has no more pending requests and is going back to sleep!");
            break;
        }
    }
    return NULL;
}

int main() {
    scanf("%d %d %d", &read_time, &write_time, &delete_time);
    scanf("%d %d %d", &num_files, &max_concurrent, &patience_time);

    for (int i = 0; i < num_files; i++) {
        files[i].readers = 0;
        files[i].writers = 0;
        files[i].is_deleted = false;
        pthread_mutex_init(&files[i].mutex, NULL);
        pthread_cond_init(&files[i].cond, NULL);
    }

    print_message(WHITE, "LAZY has woken up!\n");

    char op_str[10];
    while (true) {
        scanf("%s", op_str);
        if (strcmp(op_str, "STOP") == 0) break;
        
        requests[num_requests].user_id = atoi(op_str);
        scanf("%d %s %d", &requests[num_requests].file_id,
              op_str, &requests[num_requests].arrival_time);
        
        if (strcmp(op_str, "READ") == 0) requests[num_requests].op = READ;
        else if (strcmp(op_str, "WRITE") == 0) requests[num_requests].op = WRITE;
        else if (strcmp(op_str, "DELETE") == 0) requests[num_requests].op = DELETE;
        
        requests[num_requests].is_cancelled = false;
        requests[num_requests].is_completed = false;

        print_message(YELLOW, "User %d has made request for performing %s on file %d at %d seconds",
                     requests[num_requests].user_id,
                     get_operation_name(requests[num_requests].op),
                     requests[num_requests].file_id,
                     requests[num_requests].arrival_time);

        num_requests++;
    }

    pthread_t timekeeper_thread;
    pthread_create(&timekeeper_thread, NULL, time_keeper, NULL);

    for (int i = 0; i < num_requests; i++) {
        pthread_create(&requests[i].thread, NULL, process_request, &requests[i]);
    }

    pthread_join(timekeeper_thread, NULL);
    for (int i = 0; i < num_requests; i++) {
        pthread_join(requests[i].thread, NULL);
    }

    for (int i = 0; i < num_files; i++) {
        pthread_mutex_destroy(&files[i].mutex);
        pthread_cond_destroy(&files[i].cond);
    }

    return 0;
}