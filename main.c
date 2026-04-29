#include <stdio.h>
#include <stdlib.h>

int main(char *args[], int argc) {

    FILE* fptr;
    char filename[100];
    printf("Enter the filename: ");
    scanf("%s", filename);
    
    fptr = fopen("filename.txt", "r");

    if (fptr == NULL) {
        printf("The file is not opened.");
    }
    return 0;
}