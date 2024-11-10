#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define MAX_FILES 100
#define MAX_USERS 100
#define READ 0
#define WRITE 1
#define DELETE 2

typedef struct {
    int user_id;
    int file_id;
    int operation;
    int request_time;
} UserRequest;

typedef struct {
    int read_time;
    int write_time;
    int delete_time;
    int max_concurrent_users;
    int max_wait_time;
    int file_status[MAX_FILES]; // 0: deleted, 1: valid
    pthread_mutex_t file_locks[MAX_FILES];
    pthread_cond_t file_conditions[MAX_FILES];
    int active_readers[MAX_FILES];
    pthread_mutex_t read_write_lock[MAX_FILES];
} FileManager;

FileManager fm;

void* process_request(void* arg) {
    UserRequest req = *(UserRequest*)arg;
    free(arg);

    sleep(1); // Simulate delay before processing

    if (fm.file_status[req.file_id] == 0) {
        printf("LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested.\n", req.user_id, time(NULL));
        return NULL;
    }

    if (req.operation == READ) {
        pthread_mutex_lock(&fm.read_write_lock[req.file_id]);
        fm.active_readers[req.file_id]++;
        pthread_mutex_unlock(&fm.read_write_lock[req.file_id]);

        printf("LAZY has taken up the request of User %d at %d seconds.\n", req.user_id, time(NULL));
        sleep(fm.read_time);
        
        pthread_mutex_lock(&fm.read_write_lock[req.file_id]);
        fm.active_readers[req.file_id]--;
        if (fm.active_readers[req.file_id] == 0) {
            pthread_cond_broadcast(&fm.file_conditions[req.file_id]);
        }
        pthread_mutex_unlock(&fm.read_write_lock[req.file_id]);
        
        printf("The request for User %d was completed at %d seconds.\n", req.user_id, time(NULL));
        
    } else if (req.operation == WRITE) {
        pthread_mutex_lock(&fm.read_write_lock[req.file_id]);
        
        // Wait until there are no active readers
        while (fm.active_readers[req.file_id] > 0) {
            pthread_cond_wait(&fm.file_conditions[req.file_id], &fm.read_write_lock[req.file_id]);
        }
        
        printf("LAZY has taken up the request of User %d at %d seconds.\n", req.user_id, time(NULL));
        sleep(fm.write_time);
        
        pthread_mutex_unlock(&fm.read_write_lock[req.file_id]);
        
        printf("The request for User %d was completed at %d seconds.\n", req.user_id, time(NULL));
        
    } else if (req.operation == DELETE) {
        pthread_mutex_lock(&fm.read_write_lock[req.file_id]);
        
        // Wait until there are no active readers or writers
        while (fm.active_readers[req.file_id] > 0) {
            pthread_cond_wait(&fm.file_conditions[req.file_id], &fm.read_write_lock[req.file_id]);
        }

        if (fm.file_status[req.file_id] == 1) { // Only delete if valid
            fm.file_status[req.file_id] = 0; // Mark as deleted
            printf("The request for User %d was completed at %d seconds.\n", req.user_id, time(NULL));
        } else {
            printf("LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested.\n", req.user_id, time(NULL));
        }
        
        pthread_mutex_unlock(&fm.read_write_lock[req.file_id]);
    }

    return NULL;
}

int main() {
    // Initialize FileManager
    memset(&fm, 0, sizeof(FileManager));
    
    // Read input parameters
    int r, w, d, n, c, T;
    
    scanf("%d %d %d", &r, &w, &d);
    scanf("%d %d %d", &n, &c, &T);

    fm.read_time = r;
    fm.write_time = w;
    fm.delete_time = d;
    fm.max_concurrent_users = c;
    fm.max_wait_time = T;

    for (int i = 0; i < n; i++) {
        fm.file_status[i] = 1; // Initialize files as valid
        pthread_mutex_init(&fm.file_locks[i], NULL);
        pthread_cond_init(&fm.file_conditions[i], NULL);
        fm.active_readers[i] = 0; // No active readers initially
        pthread_mutex_init(&fm.read_write_lock[i], NULL);
    }

    printf("LAZY has woken up!\n");

    while (1) {
        UserRequest* req = malloc(sizeof(UserRequest));
        
        // Read user requests
        if (scanf("%d", &req->user_id) == EOF) break; // Stop on EOF
        scanf("%d %d %d", &req->file_id, &req->operation, &req->request_time);

        printf("User %d has made request for performing ", req->user_id);
        
        if (req->operation == READ) {
            printf("READ on file %d at %d seconds [YELLOW]\n", req->file_id, req->request_time);
            
            // Simulate waiting time before processing cancellation
            sleep(1); 
            process_request(req);
            
        } else if (req->operation == WRITE) {
            printf("WRITE on file %d at %d seconds [YELLOW]\n", req->file_id, req->request_time);
            
            // Simulate waiting time before processing cancellation
            sleep(1); 
            process_request(req);
            
        } else if (req->operation == DELETE) {
            printf("DELETE on file %d at %d seconds [YELLOW]\n", req->file_id, req->request_time);
            
            // Simulate waiting time before processing cancellation
            sleep(1); 
            process_request(req);
            
            // Simulating user cancellation after processing delay
            printf("User %d canceled the request due to no response at %ld seconds. [RED]\n", req->user_id, time(NULL));
            
        }

        char stop[5];
        scanf("%s", stop);
        if (strcmp(stop, "STOP") == 0) break; // Stop on "STOP"
    }

    printf("LAZY has no more pending requests and is going back to sleep!\n");

    // Cleanup mutexes and conditions
    for (int i = 0; i < n; i++) {
        pthread_mutex_destroy(&fm.file_locks[i]);
        pthread_cond_destroy(&fm.file_conditions[i]);
        pthread_mutex_destroy(&fm.read_write_lock[i]);
    }

    return 0;
}
