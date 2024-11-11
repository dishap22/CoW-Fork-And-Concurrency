#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_FILES 10
#define MAX_REQUESTS 100

typedef struct {
    int id;
    int deleted;
    int readers;
    int writerActive;
    pthread_mutex_t fileMutex;
    sem_t fileSemaphore;
} File;

typedef struct {
    int userId;
    int fileId;
    char operation[10];
    int arrivalTime;
} Request;

File files[MAX_FILES];
Request requestQueue[MAX_REQUESTS];
int requestCount = 0;
int readTime, writeTime, deleteTime, maxConcurrentUsers, patienceTime;

pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;

void* handleRequest(void* arg) {
    Request* request = (Request*) arg;
    int fileId = request->fileId - 1;
    File* file = &files[fileId];

    pthread_mutex_lock(&file->fileMutex);
    printf("User %d has made request for performing %s on file %d at %d seconds [YELLOW]\n",
           request->userId, request->operation, request->fileId, request->arrivalTime);
    
    sleep(1);  // Simulate LAZY wait time
    
    if (file->deleted) {
        printf("LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested. [WHITE]\n",
               request->userId, request->arrivalTime + 1);
        pthread_mutex_unlock(&file->fileMutex);
        return NULL;
    }

    if (strcmp(request->operation, "READ") == 0) {
        file->readers++;
        printf("LAZY has taken up the request of User %d at %d seconds [PINK]\n", 
               request->userId, request->arrivalTime + 1);
        pthread_mutex_unlock(&file->fileMutex);

        sleep(readTime); // Simulate reading time

        pthread_mutex_lock(&file->fileMutex);
        file->readers--;
        printf("The request for User %d was completed at %d seconds [GREEN]\n",
               request->userId, request->arrivalTime + 1 + readTime);
        pthread_mutex_unlock(&file->fileMutex);
    }
    else if (strcmp(request->operation, "WRITE") == 0) {
        while (file->writerActive || file->readers > 0) {
            pthread_mutex_unlock(&file->fileMutex);
            sleep(1); // Wait until conditions allow for writing
            pthread_mutex_lock(&file->fileMutex);
        }
        file->writerActive = 1;
        printf("LAZY has taken up the request of User %d at %d seconds [PINK]\n", 
               request->userId, request->arrivalTime + 1);
        pthread_mutex_unlock(&file->fileMutex);

        sleep(writeTime); // Simulate writing time

        pthread_mutex_lock(&file->fileMutex);
        file->writerActive = 0;
        printf("The request for User %d was completed at %d seconds [GREEN]\n",
               request->userId, request->arrivalTime + 1 + writeTime);
        pthread_mutex_unlock(&file->fileMutex);
    }
    else if (strcmp(request->operation, "DELETE") == 0) {
        while (file->writerActive || file->readers > 0) {
            pthread_mutex_unlock(&file->fileMutex);
            sleep(1); // Wait until file can be deleted
            pthread_mutex_lock(&file->fileMutex);
        }
        file->deleted = 1;
        printf("LAZY has taken up the request of User %d at %d seconds [PINK]\n", 
               request->userId, request->arrivalTime + 1);
        printf("The request for User %d was completed at %d seconds [GREEN]\n",
               request->userId, request->arrivalTime + 1 + deleteTime);
        pthread_mutex_unlock(&file->fileMutex);
    }

    return NULL;
}

void simulateLazy() {
    printf("LAZY has woken up!\n");
    pthread_t threads[MAX_REQUESTS];

    for (int i = 0; i < requestCount; i++) {
        pthread_create(&threads[i], NULL, handleRequest, &requestQueue[i]);
        sleep(1);  // Simulate request arrival time
    }

    for (int i = 0; i < requestCount; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("LAZY has no more pending requests and is going back to sleep!\n");
}

int main() {
    printf("Enter the time taken for READ, WRITE, and DELETE operations:\n");
    scanf("%d %d %d", &readTime, &writeTime, &deleteTime);

    printf("Enter the number of files:\n");
    int nFiles;
    scanf("%d", &nFiles);

    printf("Enter the maximum number of concurrent users per file:\n");
    scanf("%d", &maxConcurrentUsers);

    printf("Enter the maximum patience time for users (seconds):\n");
    scanf("%d", &patienceTime);

    // Initialize files
    for (int i = 0; i < nFiles; i++) {
        files[i].id = i + 1;
        files[i].deleted = 0;
        files[i].readers = 0;
        files[i].writerActive = 0;
        pthread_mutex_init(&files[i].fileMutex, NULL);
        sem_init(&files[i].fileSemaphore, 0, maxConcurrentUsers);
    }

    printf("Enter user requests in the format 'userId fileId operation arrivalTime'. Type 'STOP' to end input.\n");

    char input[100];
    while (1) {
        scanf(" %[^\n]", input);
        if (strcmp(input, "STOP") == 0) {
            break;
        }
        int userId, fileId, arrivalTime;
        char operation[10];
        sscanf(input, "%d %d %s %d", &userId, &fileId, operation, &arrivalTime);
        
        requestQueue[requestCount++] = (Request){userId, fileId, "", arrivalTime};
        strcpy(requestQueue[requestCount - 1].operation, operation);
    }

    simulateLazy();

    for (int i = 0; i < nFiles; i++) {
        pthread_mutex_destroy(&files[i].fileMutex);
        sem_destroy(&files[i].fileSemaphore);
    }

    return 0;
}
