#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_FILES 100
#define MAX_ID 100000

// Struct to hold file information
typedef struct {
    char name[128];
    int id;
    char timestamp[20];
} File;

void read_input(File files[], int *num_files, char *sort_column) {
    fscanf(stdin, "%d", num_files); // Read number of files
    for (int i = 0; i < *num_files; i++) {
        fscanf(stdin, "%s %d %s", files[i].name, &files[i].id, files[i].timestamp);
    }
    fscanf(stdin, "%s", sort_column); // Read the sort column
}

void counting_sort(File files[], int num_files, int sort_column, int *sorted_indices) {
    int count[MAX_ID + 1] = {0}; // Initialize count array for IDs (range 0 to MAX_ID)
    int output[num_files];

    for (int i = 0; i < num_files; i++) {
        count[files[i].id] = i + 1;
    }

    int index = 0;
    for(int i = 0; i < MAX_ID + 1; i++) {
        if(count[i] != 0) {
            sorted_indices[index++] = count[i] - 1;
        }
    }
    
}

void print_sorted_files(File files[], int num_files, int *sorted_indices, const char *sort_column) {
    printf("%s\n", sort_column);
    for (int i = 0; i < num_files; i++) {
        int index = sorted_indices[i];
        printf("%s %d %s\n", files[index].name, files[index].id, files[index].timestamp);
    }
}

int main() {
    File files[MAX_FILES];
    int num_files;
    char sort_column[20];

    // Read input data directly from stdin
    read_input(files, &num_files, sort_column);

    // Array to hold the sorted indices
    int sorted_indices[num_files];

    // Perform distributed counting sort
    counting_sort(files, num_files, 0, sorted_indices);

    // Print the sorted files based on the sorted indices
    print_sorted_files(files, num_files, sorted_indices, sort_column);

    return 0;
}
