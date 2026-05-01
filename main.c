#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#define MAX_NAME_LEN 256
#define MAX_FILES 64

int create_file(const char *name);
int open_file(const char *name);
int close_file(const char *name);
int search_file(const char *name);

struct File{
    char name[MAX_NAME_LEN];
    int is_open;
    int size;
};

p_thread_mutex_t mutex_lock;
struct File files[MAX_FILES];
sem_t file_semaphore;

int main( int argc, char *args[]) {

    FILE* fptr;
    char filename[100];

    printf("Enter the filename: ");
    scanf("%s", filename);
    
    fptr = fopen(filename, "r");

    if (fptr == NULL) {
        printf("The file could not be opened.");
        return 1;
    }

    fclose(fptr);
    return 0;
}

int create_file(const char *name) {
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].name[0] == '\0') {
            strncpy(files[i].name, name, MAX_NAME_LEN);
            files[i].is_open = 0;
            files[i].size = 0;
            pthread_mutex_unlock(&mutex_lock);
            return 0; 
        }
    }
    pthread_mutex_unlock(&mutex_lock);
    return -1; 
}