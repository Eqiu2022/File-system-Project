#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define MAX_NAME_LEN 256

int create_file(const char *name);
int open_file(const char *name);
int close_file(const char *name);
int search_file(const char *name);

struct File{
    char name[MAX_NAME_LEN];
    int is_open;
    int size;
};

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