#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

#define MAX_USERS 100
#define MAX_FILES 10
#define MAX_REQUESTS 100

#define YELLOW "\033[33m"
#define PINK "\033[35m"
#define WHITE "\033[37m"
#define GREEN "\033[32m"
#define RED "\033[31m"
#define RESET "\033[0m"

typedef struct {
    int user_id;
    int file_id;
    char operation[10];
    int request_time;
    int maxtime;
    int process_time;
} Request;

typedef struct {
    int read_time;
    int write_time;
    int delete_time;
    int num_files;
    int max_concurrent;
    int max_wait_time;
    Request requests[MAX_REQUESTS];
    int request_count;
} LAZY;

pthread_mutex_t file_mutexes[MAX_FILES];
pthread_cond_t file_conditions[MAX_FILES];
sem_t file_semaphores[MAX_FILES];
sem_t file_write_semaphores[MAX_FILES];
int file_status[MAX_FILES];
int userqueue[MAX_FILES];

void* process_read(void* arg) {
    Request* req = (Request*)arg;
    if (file_status[req->file_id] == 2) {
        printf(WHITE "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested.\n" RESET, req->user_id, req->request_time);
        return NULL;
    }

    pthread_mutex_lock(&file_mutexes[req->file_id]);
    userqueue[req->file_id]++;
    pthread_mutex_unlock(&file_mutexes[req->file_id]);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += req->maxtime;
    ts.tv_nsec = 0;

    printf(PINK "LAZY has taken up the request of User %d at %d seconds\n" RESET, req->user_id, req->request_time + 1);
    sleep(1);

    if (sem_timedwait(&file_semaphores[req->file_id], &ts) == -1) {
        printf(RED "Read request by User %d timed out.\n" RESET, req->user_id);
        pthread_mutex_lock(&file_mutexes[req->file_id]);
        userqueue[req->file_id]--;
        pthread_mutex_unlock(&file_mutexes[req->file_id]);
        return NULL;
    }

    sleep(req->process_time);
    printf(GREEN "The request for User %d was completed at %d seconds\n" RESET, req->user_id, req->request_time + 1 + req->process_time);
    sem_post(&file_semaphores[req->file_id]);

    pthread_mutex_lock(&file_mutexes[req->file_id]);
    userqueue[req->file_id]--;
    if (userqueue[req->file_id] == 0)
        pthread_cond_signal(&file_conditions[req->file_id]);
    pthread_mutex_unlock(&file_mutexes[req->file_id]);

    return NULL;
}

void* process_write(void* arg) {
    Request* req = (Request*)arg;
    if (file_status[req->file_id] == 2) {
        printf(WHITE "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested.\n" RESET, req->user_id, req->request_time);
        return NULL;
    }

    pthread_mutex_lock(&file_mutexes[req->file_id]);
    userqueue[req->file_id]++;
    pthread_mutex_unlock(&file_mutexes[req->file_id]);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += req->maxtime;
    ts.tv_nsec = 0;

    printf(PINK "LAZY has taken up the request of User %d at %d seconds\n" RESET, req->user_id, req->request_time + 1);
    sleep(1);

    if (sem_timedwait(&file_semaphores[req->file_id], &ts) == -1) {
        printf(RED "Write request by User %d timed out while waiting for file semaphore.\n" RESET, req->user_id);
        pthread_mutex_lock(&file_mutexes[req->file_id]);
        userqueue[req->file_id]--;
        pthread_mutex_unlock(&file_mutexes[req->file_id]);
        return NULL;
    }

    if (sem_timedwait(&file_write_semaphores[req->file_id], &ts) == -1) {
        printf(RED "Write request by User %d timed out while waiting for write semaphore.\n" RESET, req->user_id);
        sem_post(&file_semaphores[req->file_id]);
        pthread_mutex_lock(&file_mutexes[req->file_id]);
        userqueue[req->file_id]--;
        pthread_mutex_unlock(&file_mutexes[req->file_id]);
        return NULL;
    }

    file_status[req->file_id] = 1;
    sleep(req->process_time);
    printf(GREEN "The request for User %d was completed at %d seconds\n" RESET, req->user_id, req->request_time + 1 + req->process_time);
    file_status[req->file_id] = 0;
    sem_post(&file_write_semaphores[req->file_id]);
    sem_post(&file_semaphores[req->file_id]);

    pthread_mutex_lock(&file_mutexes[req->file_id]);
    userqueue[req->file_id]--;
    if (userqueue[req->file_id] == 0)
        pthread_cond_signal(&file_conditions[req->file_id]);
    pthread_mutex_unlock(&file_mutexes[req->file_id]);

    return NULL;
}

void* process_delete(void* arg) {
    Request* req = (Request*)arg;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += req->maxtime;
    ts.tv_nsec = 0;

    sleep(1);

    pthread_mutex_lock(&file_mutexes[req->file_id]);
    while (userqueue[req->file_id] > 0) {
        if (pthread_cond_timedwait(&file_conditions[req->file_id], &file_mutexes[req->file_id], &ts) < 0) {
            printf(RED "Delete request by User %d timed out.\n" RESET, req->user_id);
            pthread_mutex_unlock(&file_mutexes[req->file_id]);
            return NULL;
        }
    }

    if (file_status[req->file_id] == 0) {
        printf(PINK "LAZY has taken up the request of User %d at %d seconds\n" RESET, req->user_id, req->request_time + 1);
        file_status[req->file_id] = 2;
        sleep(req->process_time);
        printf("File %d has been deleted.\n", req->file_id);
        printf(GREEN "The request for User %d was completed at %d seconds\n" RESET, req->user_id, req->request_time + 1 + req->process_time);
    }
    pthread_mutex_unlock(&file_mutexes[req->file_id]);

    return NULL;
}


void* process_request(void* arg) {
    LAZY* lazy = (LAZY*)arg;
    pthread_t threads[MAX_REQUESTS];
    int prevtime = 0;

    for (int i = 0; i < lazy->request_count; i++) {
        Request* req = &lazy->requests[i];
        int duration = (lazy->requests[i].request_time - prevtime);
        if(duration>0)sleep(duration);
        printf(YELLOW "User %d has made request for performing %s on file %d at %d seconds\n" RESET, req->user_id, req->operation, req->file_id, req->request_time);

        if (strcmp(req->operation, "READ") == 0) {
            req->process_time = lazy->read_time;
            pthread_create(&threads[i], NULL, process_read, (void*)req);
        } else if (strcmp(req->operation, "WRITE") == 0) {
            req->process_time = lazy->write_time;
            pthread_create(&threads[i], NULL, process_write, (void*)req);
        } else if (strcmp(req->operation, "DELETE") == 0) {
            req->process_time = lazy->delete_time;
            pthread_create(&threads[i], NULL, process_delete, (void*)req);
        }
        prevtime = lazy->requests[i].request_time;
    }

    for (int i = 0; i < lazy->request_count; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("LAZY has no more pending requests and is going back to sleep!\n");
    return NULL;
}

int main() {
    LAZY lazy;
    printf("Enter read, write, delete times: ");
    scanf("%d %d %d", &lazy.read_time, &lazy.write_time, &lazy.delete_time);
    printf("Enter number of files, max concurrent users, max wait time: ");
    scanf("%d %d %d", &lazy.num_files, &lazy.max_concurrent, &lazy.max_wait_time);

    for (int i = 0; i < lazy.num_files; i++) {
        pthread_mutex_init(&file_mutexes[i], NULL);
        pthread_cond_init(&file_conditions[i], NULL);
        sem_init(&file_semaphores[i], 0, lazy.max_concurrent);
        sem_init(&file_write_semaphores[i], 0, 1);
        file_status[i] = 0;
        userqueue[i] = 0;
    }

    lazy.request_count = 0;
    printf("Enter user requests (user_id file_id operation request_time), type STOP to end:\n");
    while (1) {
        Request req;
        char operation[10];
        scanf("%s", operation);
        if (strcmp(operation, "STOP") == 0) break;
        req.user_id = atoi(operation);
        scanf("%d %s %d", &req.file_id, req.operation, &req.request_time);
        req.maxtime = lazy.max_wait_time;
        lazy.requests[lazy.request_count++] = req;
    }

    printf("LAZY has woken up!\n");
    pthread_t thread;
    pthread_create(&thread, NULL, process_request, (void*)&lazy);
    pthread_join(thread, NULL);

    for (int i = 0; i < lazy.num_files; i++) {
        pthread_mutex_destroy(&file_mutexes[i]);
        pthread_cond_destroy(&file_conditions[i]);
        sem_destroy(&file_semaphores[i]);
        sem_destroy(&file_write_semaphores[i]);
    }

    return 0;
}
