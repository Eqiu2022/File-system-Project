#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#define MAX_NAME_LEN 256
#define MAX_FILES 6
#define MAX_OPEN_FILES 2

int create_file(const char *name);
int open_file(const char *name);
int close_file(const char *name);
int search_file(const char *name);
int write_file(const char *name, const char *data);

void *worker(void *arg);

struct File {
    char name[MAX_NAME_LEN];
    char op[16];
    int is_open;
    char data[1024];
};

pthread_mutex_t mutex_lock;
struct File files[MAX_FILES];
sem_t open_sem;

int main(int argc, char *argv[])
{
    pthread_mutex_init(&mutex_lock, NULL);
    sem_init(&open_sem, 0, MAX_OPEN_FILES);

    struct File tasks[MAX_FILES];
    pthread_t tids[MAX_FILES];
    int count = 0;
    char choice[4];

    while(1){
        printf("Enter operation (create/open/close/search/write): ");
        scanf("%s", tasks[count].op);
        getchar();
        if(strcmp(tasks[count].op, "exit") == 0){
            break;
        }
        printf("Enter filename: ");
        scanf("%s", tasks[count].name);
        getchar();
        count++;
    } 

    for (int i = 0; i < count; i++){
        pthread_create(&tids[i], NULL, worker, &tasks[i]);
    }

    for (int i = 0; i < count; i++){
        pthread_join(tids[i], NULL);
    }

    return 0;
}

int create_file(const char *name) {
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].name[0] == '\0') {
            strncpy(files[i].name, name, MAX_NAME_LEN - 1);
            files[i].name[MAX_NAME_LEN - 1] = '\0';
            files[i].is_open = 0;
            pthread_mutex_unlock(&mutex_lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&mutex_lock);
    return -1;
}

int open_file(const char *name) {
    sem_wait(&open_sem);
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            if (files[i].is_open) {
                pthread_mutex_unlock(&mutex_lock);
                sem_post(&open_sem);
                return -1;
            }
            files[i].is_open = 1;
            pthread_mutex_unlock(&mutex_lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&mutex_lock);
    sem_post(&open_sem);
    return -1;
}

int close_file(const char *name) {
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            if (!files[i].is_open) {
                pthread_mutex_unlock(&mutex_lock);
                return -1;
            }
            files[i].is_open = 0;
            pthread_mutex_unlock(&mutex_lock);
            sem_post(&open_sem);
            return 0;
        }
    }
    pthread_mutex_unlock(&mutex_lock);
    return -1;
}

int search_file(const char *name) {
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            printf("File '%s' found. open=%d\n",
                   name, files[i].is_open);
            pthread_mutex_unlock(&mutex_lock);
            return i;
        }
    }
    printf("File '%s' not found.\n", name);
    pthread_mutex_unlock(&mutex_lock);
    return -1;
}

int write_file(const char *name, const char *data) {
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            if (!files[i].is_open) {   
                printf("File '%s' is not open for writing.\n", name);      
                pthread_mutex_unlock(&mutex_lock);
                return -1;
            }
            strncpy(files[i].data, data, sizeof(files[i].data) - 1);
            files[i].data[sizeof(files[i].data) - 1] = '\0';
            pthread_mutex_unlock(&mutex_lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&mutex_lock);
    return -1;
}


void *worker(void *arg) {
    struct File *f = (struct File *)arg;

    if (strcmp(f->op, "create") == 0) {
        if (create_file(f->name) == 0)
            printf("File '%s' created successfully.\n", f->name);
        else
            printf("Failed to create file '%s'.\n", f->name);
    } else if (strcmp(f->op, "open") == 0) {
        if (open_file(f->name) == 0)
            printf("File '%s' opened successfully.\n", f->name);
        else
            printf("Failed to open file '%s'.\n", f->name);
    } else if (strcmp(f->op, "close") == 0) {
        if (close_file(f->name) == 0)
            printf("File '%s' closed successfully.\n", f->name);
        else
            printf("Failed to close file '%s'.\n", f->name);
    } else if (strcmp(f->op, "search") == 0) {
        search_file(f->name);
    } else if (strcmp(f->op, "write") == 0) {
        printf("Enter data to write: ");
        fgets(f->data, sizeof(f->data), stdin);
        if (write_file(f->name, f->data) == 0)
            printf("Data written to file '%s' successfully.\n", f->name);
        else
            printf("Failed to find file '%s'.\n", f->name);
    }
    else {
        printf("Unknown operation '%s'.\n", f->op);
    }
    return NULL;
}