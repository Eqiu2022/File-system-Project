#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <dirent.h>

#define MAX_NAME_LEN 256
#define MAX_FILES 4
#define MAX_OPEN_FILES 2

int create_file(const char *name);
int open_file(const char *name);
int close_file(const char *name);
int search_file(const char *name);
int write_file(const char *name, const char *data);
int delete_file(const char *name);
int read_file(const char *name);
void print_files();
void *worker(void *arg);

struct File {
    char name[MAX_NAME_LEN];
    char op[16];
    FILE *fp;
};

pthread_mutex_t mutex_lock;
struct File files[MAX_FILES];
sem_t open_sem;
int open_count = 0;

int main(int argc, char *argv[])
{
    pthread_mutex_init(&mutex_lock, NULL);
    sem_init(&open_sem, 0, MAX_OPEN_FILES);

    struct File tasks[MAX_FILES];
    pthread_t tids[MAX_FILES];
    int count = 0;

    DIR *dir = opendir("files/");
    struct dirent *entry;
    int i = 0;

    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL && i < MAX_FILES) {
            if (entry->d_type == DT_REG) {
                strncpy(files[i].name, entry->d_name, MAX_NAME_LEN - 1);
                files[i].name[MAX_NAME_LEN - 1] = '\0';
                files[i].fp = NULL;
                i++;
            }
        }
        closedir(dir);
    }
    print_files();
    while(1){
        printf("Enter operation (create/open/close/search/read/write/delete/exit): ");
        scanf("%s", tasks[count].op);
        getchar();
        if(strcmp(tasks[count].op, "exit") == 0){
            print_files();
            break;
        }
        printf("Enter filename: ");
        scanf("%s", tasks[count].name);
        getchar();

        pthread_create(&tids[count], NULL, worker, &tasks[count]);
        pthread_join(tids[count], NULL);
       
        if(count < MAX_FILES)
            count++;
    }

    return 0;
}

int create_file(const char *name) {
    pthread_mutex_lock(&mutex_lock);
    char path[MAX_NAME_LEN];
    snprintf(path, sizeof(path), "files/%s", name);
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        pthread_mutex_unlock(&mutex_lock);
        return -1;
    }
    fclose(fp);
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].name[0] == '\0') {
            strncpy(files[i].name, name, MAX_NAME_LEN - 1);
            files[i].name[MAX_NAME_LEN - 1] = '\0';
            files[i].fp = NULL;
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
    if (open_count >= MAX_OPEN_FILES) {
        pthread_mutex_unlock(&mutex_lock);
        return -1;
    }
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            if (files[i].fp != NULL) {
                pthread_mutex_unlock(&mutex_lock);
                sem_post(&open_sem);
                return -1;
            }
            char path[MAX_NAME_LEN];
            snprintf(path, sizeof(path), "files/%s", name);
            files[i].fp = fopen(path, "r+");
            if (files[i].fp == NULL) {
                pthread_mutex_unlock(&mutex_lock);
                sem_post(&open_sem);
                return -1;
            }
            open_count++;
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
            if (files[i].fp == NULL) {
                printf("File '%s' is not open.\n", name);
                pthread_mutex_unlock(&mutex_lock);
                return -1;
            }
            fclose(files[i].fp);
            files[i].fp = NULL;
            open_count--;
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
            printf("File '%s' found. open=%s\n", name, files[i].fp != NULL ? "yes" : "no");
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
            if (files[i].fp == NULL) {
                printf("File '%s' is not open for writing.\n", name);
                pthread_mutex_unlock(&mutex_lock);
                return -1;
            }
            fprintf(files[i].fp, "%s", data);
            fflush(files[i].fp);
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
            if (files[i].fp != NULL) {
                fclose(files[i].fp);
                files[i].fp = NULL;
                open_count--;
                sem_post(&open_sem);
            }
            char path[MAX_NAME_LEN];
            snprintf(path, sizeof(path), "files/%s", name);
            remove(path);
            files[i].name[0] = '\0';
            pthread_mutex_unlock(&mutex_lock);
            return 0;
        }
    }
    printf("File '%s' not found for deletion.\n", name);
    pthread_mutex_unlock(&mutex_lock);
    return -1;
}

void print_files() {
    printf("\n--- File Registry ---\n");
    int found = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].name[0] != '\0') {
            printf("  [%d] %s | open=%s\n", i, files[i].name,
                   files[i].fp != NULL ? "yes" : "no");
            found = 1;
        }
    }
    if (!found)
        printf("  (empty)\n");
    printf("---------------------\n\n");
}

int read_file(const char *name) {
    pthread_mutex_lock(&mutex_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(files[i].name, name) == 0) {
            if (files[i].fp == NULL) {
                printf("File '%s' is not open for reading.\n", name);
                pthread_mutex_unlock(&mutex_lock);
                return -1;
            }
            char line[1024];
            printf("--- Contents of '%s' ---\n", name);
            rewind(files[i].fp);
            while (fgets(line, sizeof(line), files[i].fp) != NULL)
                printf("%s", line);
            printf("\n------------------------\n");
            pthread_mutex_unlock(&mutex_lock);
            return 0;
        }
    }
    printf("File '%s' not found.\n", name);
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
        if(open_count >= MAX_OPEN_FILES){
            printf("Cannot open file '%s'. Maximum open files reached.\n", f->name);
            return NULL;
        }
        open_file(f->name);
    } else if (strcmp(f->op, "close") == 0) {
        close_file(f->name);
    } else if (strcmp(f->op, "search") == 0) {
        search_file(f->name);
    } else if (strcmp(f->op, "write") == 0) {
        printf("Enter data to write: ");
        char data[1024];
        fgets(data, sizeof(data), stdin);
        write_file(f->name, data);
    } else if (strcmp(f->op, "read") == 0) {
        read_file(f->name);
    } else if (strcmp(f->op, "delete") == 0) {
        delete_file(f->name);
    } else {
        printf("Unknown operation '%s'.\n", f->op);
    }
    print_files();
    return NULL;
}