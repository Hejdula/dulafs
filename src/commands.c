#include <stdint.h>
#include <stdio.h>
#include "commands.h"
#include "dulafs.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>



// Command function implementations
int cmd_format(int argc, char** argv) {

    printf("Format function called\n"); 
    if(argc != 2){
        return 1;
    }

    char* endptr;
    long size = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || size <= 0) {
        fprintf(stderr, "Invalid size argument: %s\n", argv[1]);
        return 1;
    }
    return format((int)size);
}
int cmd_cp(int argc, char** argv) { printf("TODO: Copy function called\n"); return 0; }
int cmd_mv(int argc, char** argv) { printf("TODO: Move function called\n"); return 0; }
int cmd_rm(int argc, char** argv) { printf("TODO: Remove function called\n"); return 0; }

int cmd_mkdir(int argc, char** argv) {
    
    // separate name of the dir from path
    char target_path[MAX_DIR_PATH];
    char* dir_name;
    strcpy(target_path, argv[1]);
    char* last_slash_ptr = strrchr(target_path, '/');
    if (last_slash_ptr) {
        dir_name = last_slash_ptr + 1;
        target_path[last_slash_ptr - target_path] = '\0';
    } else {
        dir_name = argv[1];
        target_path[0] = '\0';
    }

    int target_node_id = path_to_inode(target_path);
    struct inode target_inode = get_inode(target_node_id);
    if (target_node_id == -1) return EXIT_FAILURE;
    if (target_inode.is_file) {
        printf("target destination is a file");
        return EXIT_FAILURE;
    }
    int new_node_id = create_dir_node(target_node_id); 
    struct directory_item dir_record = {0};
    dir_record.inode = new_node_id;
    strncpy(dir_record.item_name, dir_name, sizeof(dir_record.item_name) - 1);
    dir_record.item_name[sizeof(dir_record.item_name) - 1] = '\0';
    struct inode curr_node = get_inode(target_node_id); 
    add_record_to_dir(dir_record, &curr_node);

    return EXIT_SUCCESS;
}

int cmd_rmdir(int argc, char** argv) {

    if (!strcmp(argv[1],".") || !strcmp(argv[1],"..")){
        printf("can not remove \".\" or \"..\" from directory");
        return EXIT_FAILURE;
    }

    char target_path[MAX_DIR_PATH];
    strcpy(target_path, argv[1]);
    
    // separate name of the dir from path and strip it from target path
    char* last_slash = strrchr(argv[1], '/');
    char* dir_name;

    // strip the name from target path
    if (last_slash) {
        dir_name = last_slash + 1;
        size_t index = last_slash - argv[1];
        target_path[index] = '\0';
    } else {
        dir_name = argv[1];
        target_path[0] = '\0';
    }    
    int target_node_id = path_to_inode(target_path);
    struct inode target_inode = get_inode(target_node_id);
    delete_item(&target_inode, dir_name);

    return EXIT_SUCCESS;
}
    
int cmd_ls(int argc, char** argv) {
    struct inode curr_inode = get_inode(g_system_state.curr_node_id);
    struct directory_item* dir_content = (struct directory_item*) get_node_data(&curr_inode);
    int record_count = curr_inode.file_size / sizeof(struct directory_item);
    for (int i = 0; i < record_count; i++){
        if (dir_content[i].item_name[0]){
            printf("%s : %d\n",dir_content[i].item_name, dir_content[i].inode);
        }
    }

    free(dir_content);
    return EXIT_SUCCESS;
}

int cmd_cat(int argc, char** argv) { 
    int node_id = path_to_inode(argv[1]);
    if (node_id == -1) return EXIT_FAILURE;
    struct inode inode = get_inode(node_id);
    uint8_t* data = (uint8_t*)get_node_data(&inode);
    // printf("inode size: %d\n", inode.file_size);
    for (int i = 0; i < inode.file_size; i++) {
        if (data[i] == 0){
            printf("\xE2\x96\xA1"); // Unicode white square (U+25A1) in UTF-8 to represent zero byte
        } else {
            putchar(data[i]);
        }
    }
    putchar('\n');

    free(data);
    return EXIT_SUCCESS;
}

int cmd_cd(int argc, char** argv) {
    int new_node_id = path_to_inode(argv[1]);
    if (new_node_id == -1){
        return EXIT_FAILURE;
    }
    struct inode node = get_inode(new_node_id);
    if (node.is_file){
        fprintf(stderr, "%s not a directory", argv[1]);
        return EXIT_FAILURE;
    }

    g_system_state.curr_node_id = new_node_id;
    char* new_path = inode_to_path(g_system_state.curr_node_id);
    strlcpy(g_system_state.working_dir, new_path, sizeof(g_system_state.working_dir) - 1);
    free(new_path);

    return EXIT_SUCCESS;
}

int cmd_pwd(int argc, char** argv) {
    char* path = inode_to_path(g_system_state.curr_node_id);
    if(path == NULL) return EXIT_FAILURE;
    printf("working directory: %s\n", path);
    free(path);
    return EXIT_SUCCESS;
}

int cmd_info(int argc, char** argv) {
    
    return EXIT_SUCCESS;
}

int cmd_incp(int argc, char** argv) {

    FILE* fptr = fopen(argv[1],"r");

    if (!fptr){
        printf("File not found");
        return EXIT_FAILURE;
    }
    
    fseek(fptr, 0, SEEK_END);
    int file_size = ftell(fptr);
    
    
    // initialize the file inode
    
    int new_node_id = assign_empty_inode();
    struct inode inode = {0};
    inode.id = new_node_id;
    inode.is_file = 1;
    inode.file_size = file_size;
    write_inode(&inode);

    // separate name of the file from path
    char target_path[MAX_DIR_PATH];
    strcpy(target_path, argv[2]);
    
    char* last_slash = strrchr(argv[2], '/');
    char* file_name;

    if (last_slash) {
        file_name = last_slash + 1;
        size_t index = last_slash - argv[2];
        target_path[index] = '\0';
    } else {
        file_name = argv[2];
        target_path[0] = '\0';
    }    

    // get node_id of the directory
    int target_dir_id = path_to_inode(target_path);
    printf("target_path: %s", target_path);
    if(target_dir_id == -1){
        delete_inode(&inode);
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // add the file into directory
    struct inode target_dir = get_inode(target_dir_id);
    struct directory_item item = {0};
    item.inode = inode.id;
    strlcpy(item.item_name, file_name, sizeof(item.item_name));
    add_record_to_dir(item, &target_dir);

    // assign clusters to this inode
    int* clusters = assign_node_clusters(&inode);

    // write the file data into clusters
    uint8_t current_cluster_data[CLUSTER_SIZE];
    int cluster_count = (inode.file_size + CLUSTER_SIZE - 1) / CLUSTER_SIZE;
    
    rewind(fptr);
    for (int i = 0; i < cluster_count; i++){
        // Zero out the buffer to avoid writing uninitialized data
        memset(current_cluster_data, 0, CLUSTER_SIZE);
        
        // Calculate bytes to read for this cluster
        int bytes_to_read = CLUSTER_SIZE;
        if (i == cluster_count - 1 && file_size % CLUSTER_SIZE != 0) {
            bytes_to_read = file_size % CLUSTER_SIZE;
        }
        
        fread(current_cluster_data, 1, bytes_to_read, fptr); 
        
        int offset = clusters[i] * CLUSTER_SIZE + g_system_state.sb.data_start_address;
        fseek(g_system_state.file_ptr, offset, SEEK_SET);
        fwrite(current_cluster_data, 1, CLUSTER_SIZE, g_system_state.file_ptr);
    }
    fflush(g_system_state.file_ptr);



    fclose(fptr);
    free(clusters);
    return EXIT_SUCCESS;
}

int cmd_outcp(int argc, char** argv) {
    FILE* fptr = fopen(argv[2], "w+");
    if(!fptr) return EXIT_FAILURE;

    int file_node_id= path_to_inode(argv[1]);
    struct inode file_inode = get_inode(file_node_id);

    uint8_t* data = get_node_data(&file_inode);

    if(!fwrite(data, file_inode.file_size, 1,fptr)) return EXIT_FAILURE;

    fclose(fptr);

    return EXIT_SUCCESS;
}
int cmd_load(int argc, char** argv) { printf("TODO: Load function called\n"); return 0; }
int cmd_statfs(int argc, char** argv) { printf("TODO: Status filesystem function called\n"); return 0; }

// Array of command structs - combines name and function in one place
struct CommandEntry commands[] = {
    {"format", cmd_format, 1},
    {"cp", cmd_cp, 2},
    {"mv", cmd_mv, 2},
    {"rm", cmd_rm, 1},
    {"mkdir", cmd_mkdir, 1},
    {"rmdir", cmd_rmdir, 1},
    {"ls", cmd_ls, -1},
    {"cat", cmd_cat, 1},
    {"cd", cmd_cd, 1},
    {"pwd", cmd_pwd, 0},
    {"info", cmd_info, 1},
    {"incp", cmd_incp, 2},
    {"outcp", cmd_outcp, 2},
    {"load", cmd_load, 0},
    {"statfs", cmd_statfs, 0},
    {"test", test, -1},
};

// Number of commands
const int NUM_COMMANDS = sizeof(commands) / sizeof(commands[0]);