#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define THRESHOLD 42

typedef struct {
    char name[129];
    int id;
    char timestamp[20]; 
} File;

typedef struct {
    File *files;
    int left;
    int right;
} MergeSortArgs;

int compare_by_name(const void *a, const void *b) {
    return strcmp(((File *)a)->name, ((File *)b)->name);
}

int compare_by_id(const void *a, const void *b) {
    return ((File *)a)->id - ((File *)b)->id;
}

int compare_by_timestamp(const void *a, const void *b) {
    return strcmp(((File *)a)->timestamp, ((File *)b)->timestamp);
}

void count_sort(File *files, int n, int (*compare)(const void *, const void *)) {
    qsort(files, n, sizeof(File), compare);
}

void merge(File *files, int left, int mid, int right, int (*compare)(const void *, const void *)) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    File *L = (File *)malloc(n1 * sizeof(File));
    File *R = (File *)malloc(n2 * sizeof(File));

    for (int i = 0; i < n1; i++) {
        L[i] = files[left + i];
    }
    for (int i = 0; i < n2; i++) {
        R[i] = files[mid + 1 + i];
    }

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (compare(&L[i], &R[j]) <= 0) {
            files[k++] = L[i++];
        } else {
            files[k++] = R[j++];
        }
    }

    while (i < n1) {
        files[k++] = L[i++];
    }
    while (j < n2) {
        files[k++] = R[j++];
    }

    free(L);
    free(R);
}

void *merge_sort(void *args) {
    MergeSortArgs *ms_args = (MergeSortArgs *)args;
    int left = ms_args->left;
    int right = ms_args->right;
    File *files = ms_args->files;
    int (*compare)(const void *, const void *) = compare_by_id; // Default comparison, should be changed dynamically

    if (left < right) {
        int mid = left + (right - left) / 2;

        MergeSortArgs left_args = {files, left, mid};
        MergeSortArgs right_args = {files, mid + 1, right};

        pthread_t left_thread, right_thread;
        pthread_create(&left_thread, NULL, merge_sort, &left_args);
        pthread_create(&right_thread, NULL, merge_sort, &right_args);

        pthread_join(left_thread, NULL);
        pthread_join(right_thread, NULL);

        merge(files, left, mid, right, compare);
    }
    return NULL;
}

void distributed_merge_sort(File *files, int n, int (*compare)(const void *, const void *)) {
    MergeSortArgs args = {files, 0, n - 1};
    merge_sort(&args);
}

int main() {
    int n;
    scanf("%d", &n);
    File *files = (File *)malloc(n * sizeof(File));

    for (int i = 0; i < n; i++) {
        scanf("%s %d %s", files[i].name, &files[i].id, files[i].timestamp);
    }

    char column[10];
    scanf("%s", column);

    int (*compare)(const void *, const void *);
    if (strcmp(column, "Name") == 0) {
        compare = compare_by_name;
    } else if (strcmp(column, "ID") == 0) {
        compare = compare_by_id;
    } else {
        compare = compare_by_timestamp;
    }

    if (n <= THRESHOLD) {
        count_sort(files, n, compare);
    } else {
        distributed_merge_sort(files, n, compare);
    }

    printf("%s\n", column);
    for (int i = 0; i < n; i++) {
        printf("%s %d %s\n", files[i].name, files[i].id, files[i].timestamp);
    }

    free(files);
    return 0;
}
