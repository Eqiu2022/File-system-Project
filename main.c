#include <stdio.h>
#include <stdlib.h>

int main( int argc, char *args[]) {

    FILE* fptr;
    char filename[100];
    
    printf("Enter the filename: ");
    scanf("%s", filename);
    
    fptr = fopen(filename, "r");

    if (fptr == NULL) {
        printf("The file is not opened.");
        return 1;
    }

    fclose(fptr)
    return 0;
}