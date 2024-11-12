#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

// ANSI color codes for printing messages in color
#define YELLOW "\033[0;33m"
#define PINK "\033[0;35m"
#define WHITE "\033[0;37m"
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

// Define structure for user requests
typedef struct {
    int user_id;
    int file_id;
    char operation[10];
    int request_time;
} Request;

// Global variables for simulation parameters
int r_time, w_time, d_time;    // Time for READ, WRITE, DELETE operations
int num_files, max_concurrent; // Number of files and max concurrent users per file
int max_wait_time;             // Max time a user will wait before cancelling
sem_t *file_locks;             // Semaphore array to manage file access
int *file_deleted;             // Array to keep track if a file is deleted

// Function prototypes for colored print statements
void printUserRequest(int user_id, const char *operation, int file_id, int timestamp);
void printLazyProcessing(int user_id, int timestamp);
void printInvalidRequest(int user_id, int timestamp);
void printRequestComplete(int user_id, int timestamp);
void printUserCancelled(int user_id, int timestamp);

// Thread function to handle individual user requests
void *process_request(void *arg) {
    Request *req = (Request *)arg;
    sleep(req->request_time); // Simulate user waiting until their request time

    // Print user request message
    printUserRequest(req->user_id, req->operation, req->file_id, req->request_time);

    // Check for file deletion status
    if (file_deleted[req->file_id - 1]) {
        printInvalidRequest(req->user_id, req->request_time + 1);
        pthread_exit(NULL);
    }

    // Acquire lock based on operation
    int wait_time = 0;
    int operation_time = strcmp(req->operation, "READ") == 0 ? r_time :
                         strcmp(req->operation, "WRITE") == 0 ? w_time : d_time;

    // Loop until the lock is acquired or max_wait_time is exceeded
    while (sem_trywait(&file_locks[req->file_id - 1]) != 0) {
        sleep(1);  // Wait 1 second between retries
        wait_time++;
        if (wait_time > max_wait_time) {  // Check if max wait time has been exceeded
            printUserCancelled(req->user_id, req->request_time + wait_time);
            pthread_exit(NULL);  // Exit the thread if the user cancels
        }
    }

    // LAZY starts processing request
    printLazyProcessing(req->user_id, req->request_time + wait_time + 1);

    // Simulate operation time
    sleep(operation_time);

    // Check if the operation is DELETE and mark the file as deleted
    if (strcmp(req->operation, "DELETE") == 0) {
        file_deleted[req->file_id - 1] = 1;
    }

    // Operation completed
    printRequestComplete(req->user_id, req->request_time + wait_time + operation_time + 1);

    // Release lock
    sem_post(&file_locks[req->file_id - 1]);
    pthread_exit(NULL);
}

// Main function to set up simulation
int main() {
    // Input parsing
    printf("Enter times for READ, WRITE, and DELETE operations:\n");
    scanf("%d %d %d", &r_time, &w_time, &d_time);

    printf("Enter number of files, max concurrent accesses, and max wait time:\n");
    scanf("%d %d %d", &num_files, &max_concurrent, &max_wait_time);

    // Allocate and initialize semaphores and file deletion tracking
    file_locks = malloc(num_files * sizeof(sem_t));
    file_deleted = malloc(num_files * sizeof(int));
    for (int i = 0; i < num_files; i++) {
        sem_init(&file_locks[i], 0, max_concurrent); // Initialize semaphore for each file
        file_deleted[i] = 0; // Initially, no file is deleted
    }

    // Array to hold requests
    Request requests[100];
    int req_count = 0;

    printf("Enter user requests (ID, file, operation, request time) and type STOP to end:\n");
    while (1) {
        char input[20];
        scanf("%s", input);
        if (strcmp(input, "STOP") == 0) break;

        requests[req_count].user_id = atoi(input);
        scanf("%d %s %d", &requests[req_count].file_id, requests[req_count].operation, &requests[req_count].request_time);
        req_count++;
    }

    printf(WHITE "LAZY has woken up!\n" RESET);

    // Create threads for each request
    pthread_t threads[req_count];
    for (int i = 0; i < req_count; i++) {
        pthread_create(&threads[i], NULL, process_request, (void *)&requests[i]);
    }

    // Join threads after completion
    for (int i = 0; i < req_count; i++) {
        pthread_join(threads[i], NULL);
    }

    printf(WHITE "LAZY has no more pending requests and is going back to sleep!\n" RESET);

    // Cleanup
    for (int i = 0; i < num_files; i++) {
        sem_destroy(&file_locks[i]);
    }
    free(file_locks);
    free(file_deleted);

    return 0;
}

// Function definitions for colored print statements
void printUserRequest(int user_id, const char *operation, int file_id, int timestamp) {
    printf(YELLOW "User %d has made request for performing %s on file %d at %d seconds\n" RESET, user_id, operation, file_id, timestamp);
}

void printLazyProcessing(int user_id, int timestamp) {
    printf(PINK "LAZY has taken up the request of User %d at %d seconds\n" RESET, user_id, timestamp);
}

void printInvalidRequest(int user_id, int timestamp) {
    printf(WHITE "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested.\n" RESET, user_id, timestamp);
}

void printRequestComplete(int user_id, int timestamp) {
    printf(GREEN "The request for User %d was completed at %d seconds\n" RESET, user_id, timestamp);
}

void printUserCancelled(int user_id, int timestamp) {
    printf(RED "User %d canceled the request due to no response at %d seconds\n" RESET, user_id, timestamp);
}
