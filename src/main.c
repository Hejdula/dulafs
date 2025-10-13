#include <asm-generic/errno-base.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
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

    FILE* file_ptr = fopen(file_path, "rb+");

    if (file_ptr == NULL){
        fprintf(stderr, "unable to open file %s", file_path);
        return ENOENT;
    }

    g_system_state.file_ptr = file_ptr ;

    if (fread(&g_system_state.sb, sizeof(struct superblock), 1, file_ptr) != 1) {
        fprintf(stderr, "Failed to read superblock from file\n");
    }

    if (strcmp(g_system_state.sb.signature, "HEJDULA")) {
        printf("The file does not have signature of .ula file(HEJDULA) and may not be properly formatted, format the file with format command");
    } else {
        printf("Valid .ula filesystem detected!\n");
        printf("\n=== Superblock Information ===\n");
        printf("Signature: '%.8s'\n", g_system_state.sb.signature);
        printf("Disk size: %d bytes\n", g_system_state.sb.disk_size);
        printf("Cluster size: %d bytes\n", g_system_state.sb.cluster_size);
        printf("Cluster count: %d\n", g_system_state.sb.cluster_count);
        printf("Inode count: %d\n", g_system_state.sb.inode_count);
        printf("Inode bitmap start address: %d\n", g_system_state.sb.bitmapi_start_address);
        printf("Cluster bitmap start address: %d\n", g_system_state.sb.bitmap_start_address);
        printf("Inode start address: %d\n", g_system_state.sb.inode_start_address);
        printf("Data start address: %d\n", g_system_state.sb.data_start_address);
        printf("===============================\n\n");
    }

    repl();
     
    fclose(file_ptr);
    
    return 0;
}
