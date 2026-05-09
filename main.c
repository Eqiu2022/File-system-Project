#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#define MAX_NAME_LEN 256
#define MAX_FILES 4
#define MAX_OPEN_FILES 2

int create_file(const char *name);
int open_file(const char *name);
int close_file(const char *name);
int search_file(const char *name);
int read_file(const char *name);
int write_file(const char *name, const char *data);
int delete_file(const char *name);
void list_files();
void *worker(void *arg);

struct File {
    char name[MAX_NAME_LEN];
    int is_open;
    char data[1024];
};

struct Task {
    char op[16];
    char name[MAX_NAME_LEN];
    char data[1024];
};

pthread_mutex_t mutex_lock;
struct File files[MAX_FILES];
sem_t open_sem;
sem_t create_sem;

int main(int argc, char *argv[])
{
    pthread_mutex_init(&mutex_lock, NULL);
    sem_init(&open_sem, 0, MAX_OPEN_FILES);
    sem_init(&create_sem, 0, MAX_FILES);

    struct Task task;
    pthread_t tid;

    printf("=== File System Simulator ===\n");
    printf("Max files: %d | Max open at once: %d\n\n", MAX_FILES, MAX_OPEN_FILES);

    while (1) {
        printf("-------------------------------\n");
        list_files();
        printf("Operations: create / open / close / read / write / search / delete / exit\n");
        printf("> ");
        scanf("%s", task.op);
        getchar();

        if (strcmp(task.op, "exit") == 0) {
            printf("Exiting.\n");
            break;
        }

        if (strcmp(task.op, "create")  != 0 &&
            strcmp(task.op, "open")    != 0 &&
            strcmp(task.op, "close")   != 0 &&
            strcmp(task.op, "read")    != 0 &&
            strcmp(task.op, "write")   != 0 &&
            strcmp(task.op, "search")  != 0 &&
            strcmp(task.op, "delete")  != 0) {
            printf("Unknown operation '%s'.\n", task.op);
            continue;
        }

        printf("Filename: ");
        scanf("%s", task.name);
        getchar();

        if (strcmp(task.op, "write") == 0) {
            printf("Data to write: ");
            fgets(task.data, sizeof(task.data), stdin);
            task.data[strcspn(task.data, "\n")] = '\0';
        }

        pthread_create(&tid, NULL, worker, &task);
        pthread_join(tid, NULL);
    }

    sem_destroy(&open_sem);
    sem_destroy(&create_sem);
    pthread_mutex_destroy(&mutex_lock);

    return 0;
}

void list_files() {
    printf("Files:\n");
    int found = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].name[0] != '\0') {
            printf("  [%d] %s (%s)\n", i, files[i].name,
                   files[i].is_open ? "open" : "closed");
            found = 1;
        }
    }
    if (!found) printf("  (none)\n");
}

int create_file(const char *name) {
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            printf("File '%s' already exists.\n", name);
            pthread_mutex_unlock(&mutex_lock);
            return -1;
        }
    }
    pthread_mutex_unlock(&mutex_lock);

    sem_wait(&create_sem);

    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].name[0] == '\0') {
            strncpy(files[i].name, name, MAX_NAME_LEN - 1);
            files[i].name[MAX_NAME_LEN - 1] = '\0';
            files[i].is_open = 0;
            files[i].data[0] = '\0';
            pthread_mutex_unlock(&mutex_lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&mutex_lock);
    sem_post(&create_sem);
    return -1;
}

int open_file(const char *name) {
    sem_wait(&open_sem);

    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            if (files[i].is_open) {
                printf("File '%s' is already open.\n", name);
                pthread_mutex_unlock(&mutex_lock);
                sem_post(&open_sem);
                return -1;
            }
            files[i].is_open = 1;
            pthread_mutex_unlock(&mutex_lock);
            return 0;
        }
    }
    printf("File '%s' not found.\n", name);
    pthread_mutex_unlock(&mutex_lock);
    sem_post(&open_sem);
    return -1;
}

int close_file(const char *name) {
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            if (!files[i].is_open) {
                printf("File '%s' is already closed.\n", name);
                pthread_mutex_unlock(&mutex_lock);
                return -1;
            }
            files[i].is_open = 0;
            pthread_mutex_unlock(&mutex_lock);
            sem_post(&open_sem);
            return 0;
        }
    }
    printf("File '%s' not found.\n", name);
    pthread_mutex_unlock(&mutex_lock);
    return -1;
}

int search_file(const char *name) {
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            printf("File '%s' found at slot %d. Status: %s\n",
                   name, i, files[i].is_open ? "open" : "closed");
            pthread_mutex_unlock(&mutex_lock);
            return i;
        }
    }
    printf("File '%s' not found.\n", name);
    pthread_mutex_unlock(&mutex_lock);
    return -1;
}

int read_file(const char *name) {
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            if (!files[i].is_open) {
                printf("File '%s' is not open for reading.\n", name);
                pthread_mutex_unlock(&mutex_lock);
                return -1;
            }
            printf("Data in '%s': %s\n", name,
                   files[i].data[0] == '\0' ? "(empty)" : files[i].data);
            pthread_mutex_unlock(&mutex_lock);
            return 0;
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
    printf("File '%s' not found.\n", name);
    pthread_mutex_unlock(&mutex_lock);
    return -1;
}

int delete_file(const char *name) {
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            if (files[i].is_open) {
                printf("File '%s' is open, close it before deleting.\n", name);
                pthread_mutex_unlock(&mutex_lock);
                return -1;
            }
            files[i].name[0] = '\0';
            files[i].is_open = 0;
            files[i].data[0] = '\0';
            pthread_mutex_unlock(&mutex_lock);
            sem_post(&create_sem);
            return 0;
        }
    }
    printf("File '%s' not found.\n", name);
    pthread_mutex_unlock(&mutex_lock);
    return -1;
}

void *worker(void *arg) {
    struct Task *t = (struct Task *)arg;

    if (strcmp(t->op, "create") == 0) {
        if (create_file(t->name) == 0)
            printf("File '%s' created successfully.\n", t->name);
        else
            printf("Failed to create '%s'.\n", t->name);
    }
    else if (strcmp(t->op, "open") == 0) {
        if (open_file(t->name) == 0)
            printf("File '%s' opened successfully.\n", t->name);
        else
            printf("Failed to open '%s'.\n", t->name);
    }
    else if (strcmp(t->op, "close") == 0) {
        if (close_file(t->name) == 0)
            printf("File '%s' closed successfully.\n", t->name);
        else
            printf("Failed to close '%s'.\n", t->name);
    }
    else if (strcmp(t->op, "search") == 0) {
        search_file(t->name);
    }
    else if (strcmp(t->op, "read") == 0) {
        if (read_file(t->name) != 0)
            printf("Failed to read '%s'.\n", t->name);
    }
    else if (strcmp(t->op, "write") == 0) {
        if (write_file(t->name, t->data) == 0)
            printf("Data written to '%s' successfully.\n", t->name);
        else
            printf("Failed to write to '%s'.\n", t->name);
    }
    else if (strcmp(t->op, "delete") == 0) {
        if (delete_file(t->name) == 0)
            printf("File '%s' deleted successfully.\n", t->name);
        else
            printf("Failed to delete '%s'.\n", t->name);
    }

    return NULL;
}