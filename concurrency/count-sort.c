#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_FILES 100
#define MAXTHREADS 4
#define ID_HASH_SIZE 1000000
#define NAME_HASH_SIZE 10000000

typedef struct {
    char name[128];
    int id;
    char timestamp[20];
} File;

typedef struct {
    File *files;
    int left_bound;
    int right_bound;
    int *count_array;
    int use_id;
} ThreadData;

unsigned int hash_string(const char *str) {
    unsigned int hash = 0;
    int constant = 31;
    int length = 4;
    unsigned int power = 1;

    for (int i = 0; i < length - 1; i++) {
        power *= constant;
    }

    for (int i = 0; str[i] != '\0'; i++) {
        hash += str[i] * power;
        power /= constant;
    }

    return hash;
}

void* count_function(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    for (int i = data->left_bound; i < data->right_bound; i++) {
        int key = data->use_id ? data->files[i].id : hash_string(data->files[i].name);
        data->count_array[key] = i + 1;
    }
    return NULL;
}

void initialize_count_array(File files[], int num_files, int *count_array, int use_id) {
    pthread_t threads[MAXTHREADS];
    ThreadData thread_data[MAXTHREADS];
    int range = num_files / MAXTHREADS;
    for (int i = 0; i < MAXTHREADS; i++) {
        thread_data[i].files = files;
        thread_data[i].count_array = count_array;
        thread_data[i].left_bound = i * range;
        thread_data[i].right_bound = (i == MAXTHREADS - 1) ? num_files : (i + 1) * range;
        thread_data[i].use_id = use_id;
        pthread_create(&threads[i], NULL, count_function, &thread_data[i]);
    }
    for (int i = 0; i < MAXTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

void print_sorted_files(File files[], int *count_array, int hash_size) {
    printf("name id timestamp\n");
    for (int i = 0; i < hash_size; i++) {
        if (count_array[i] != 0) {
            int index = count_array[i] - 1;
            printf("%s %d %s\n", files[index].name, files[index].id, files[index].timestamp);
        }
    }
}

void read_input(File files[], int *num_files, char *sort_column) {
    fscanf(stdin, "%d", num_files);
    for (int i = 0; i < *num_files; i++) {
        fscanf(stdin, "%s %d %s", files[i].name, &files[i].id, files[i].timestamp);
    }
    fscanf(stdin, "%s", sort_column);
}

int main() {
    File files[MAX_FILES];
    int *count_array;
    int num_files;
    char sort_column[20];

    read_input(files, &num_files, sort_column);
    int use_id = strcmp(sort_column, "ID") == 0;

    int hash_size = use_id ? ID_HASH_SIZE : NAME_HASH_SIZE;
    count_array = (int *)calloc(hash_size, sizeof(int));

    initialize_count_array(files, num_files, count_array, use_id);
    print_sorted_files(files, count_array, hash_size);

    free(count_array);
    return 0;
}
