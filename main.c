#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "repl.h"


int main(int argc, char* argv[]){
    if (argc != 2){
        fprintf(stderr, "Invalid number of arguments: expected 1, got %d\nUsage: %s <pathToFile.dula>\n", argc - 1, argv[0]);
        return EINVAL;
    }
    char* filePath = argv[1];

    printf("Trying to open: %s\n", filePath);

    if (access(filePath, F_OK)){
        fprintf(stderr, "File does not exist: %s\n", filePath);
        return ENOENT;
    }

    if (access(filePath, W_OK|R_OK)){
        fprintf(stderr, "Insufficient permissions to read/write to: %s\n", filePath);
        return EACCES;
    }

    FILE* fptr = fopen(filePath, "rw");

    repl();
     
    fclose(fptr);
    
    return 0;
}
