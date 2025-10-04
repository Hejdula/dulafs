#include <asm-generic/errno-base.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "repl.h"
#include "dulafs.h"


int main(int argc, char* argv[]){
    if (argc != 2){
        fprintf(stderr, "Invalid number of arguments: expected 1, got %d\nUsage: %s <pathToFile.dula>\n", argc - 1, argv[0]);
        return EINVAL;
    }
    char* file_path = argv[1];

    printf("Trying to open: %s\n", file_path);

    if (access(file_path, F_OK)){
        fprintf(stderr, "File does not exist: %s\n", file_path);
        return ENOENT;
    }
    if (access(file_path, W_OK|R_OK)){
        fprintf(stderr, "Insufficient permissions to read/write to: %s\n", file_path);
        return EACCES;
    }

    FILE* file_ptr = fopen(file_path, "rw");

    if (file_ptr == NULL){
        fprintf(stderr, "unable to open file %s", file_path);
        return ENOENT;
    }

    g_system_state.file_ptr = file_ptr ;

    repl();
     
    fclose(file_ptr);
    
    return 0;
}
