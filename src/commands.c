#include <stdint.h>
#include <stdio.h>
#include "commands.h"
#include "dulafs.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>



// Command function implementations
int cmd_format(int argc, char** argv) {

    if(argc != 2){
        return EXIT_FAILURE;
    }

    char* endptr;
    long size = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || size <= 0) {
        fprintf(stderr, "Invalid size argument: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    return format((int)size);
}
int cmd_cp(int argc, char** argv) {
    // get inode to copy
    int original_inode_id = path_to_inode(argv[1]); 
    if(original_inode_id == -1) return EXIT_FAILURE;
    struct inode original_node = get_inode(original_inode_id);

    // separate name of the file from path
    char* file_name = get_final_token(argv[2]);
    
    if (!file_name || file_name[0] == '\0') {
        fprintf(stderr, "File name cannot be empty\n");
        return EXIT_FAILURE;
    }
 
    int target_dir_id = path_to_dir_inode(argv[2]);
    if (target_dir_id == -1) return EXIT_FAILURE;
    struct inode target_dir = get_inode(target_dir_id);

    // check if file already exists
    if (contains_file(&target_dir, file_name)){
        fprintf(stderr, "Directory or file \"%s\"with the same name already exists\n", argv[2]);
        return EXIT_FAILURE;
    }

    // create new inode
    int new_inode_id = assign_empty_inode();
    if(new_inode_id == -1) return EXIT_FAILURE;
    struct inode new_inode = {0};
    new_inode.id = new_inode_id;
    new_inode.file_size = original_node.file_size;
    new_inode.is_file = 1; 
    write_inode(&new_inode);
    
    int* new_clusters = assign_node_clusters(&new_inode);
    int* original_clusters = get_node_clusters(&original_node);
    if (original_clusters == NULL) {
        clear_inode(&new_inode);
        return EXIT_FAILURE;
    }

    // copy the data to new inode
    int cluster_count = (original_node.file_size + CLUSTER_SIZE - 1) / CLUSTER_SIZE;
    printf("cluster count: %d", cluster_count);
    int offset;
    uint8_t* current_cluster_data = malloc(CLUSTER_SIZE);
    for (int cluster_index = 0; cluster_index < cluster_count; cluster_index++){ 
        // Zero out the buffer to avoid writing uninitialized data
        memset(current_cluster_data, 0, CLUSTER_SIZE);
        
        // Calculate bytes to read for this cluster
        int bytes_to_read = CLUSTER_SIZE;
        if (cluster_index == cluster_count - 1 && original_node.file_size % CLUSTER_SIZE != 0) {
            bytes_to_read = original_node.file_size % CLUSTER_SIZE;
        }
        
        
        // read from original
        printf("read from %d", original_clusters[cluster_index]);
        offset = original_clusters[cluster_index] * CLUSTER_SIZE + g_system_state.sb.data_start_address;
        fseek(g_system_state.file_ptr, offset, SEEK_SET);
        fread(current_cluster_data, bytes_to_read, 1, g_system_state.file_ptr); 
        
        // write to new one
        printf("write to %d", new_clusters[cluster_index]);
        offset = new_clusters[cluster_index] * CLUSTER_SIZE + g_system_state.sb.data_start_address;
        fseek(g_system_state.file_ptr, offset, SEEK_SET);
        fwrite(current_cluster_data, bytes_to_read, 1, g_system_state.file_ptr);
    }
    free(current_cluster_data);
    free(original_clusters);
    free(new_clusters);
    
    struct directory_item item = {0};
    item.inode = new_inode_id;
    strlcpy(item.item_name, file_name, sizeof(item.item_name));
    
    add_record_to_dir(item, &target_dir); 
    fflush(g_system_state.file_ptr);

    return EXIT_SUCCESS;
}
int cmd_mv(int argc, char** argv) { 
    char* to_file_name = get_final_token(argv[2]);
    char* from_file_name = get_final_token(argv[1]);
    
    if (!to_file_name || to_file_name[0] == '\0') {
        fprintf(stderr, "File name cannot be empty\n");
        return EXIT_FAILURE;
    }
    
    // source directory
    int from_dir_id = path_to_dir_inode(argv[1]); 
    if (from_dir_id == -1){ return EXIT_FAILURE; }
    
    // destination directory
    int to_dir_id = path_to_dir_inode(argv[2]);
    if (to_dir_id == -1){ return EXIT_FAILURE; }
    struct inode to_dir_inode = get_inode(to_dir_id); 
    
    // source file
    int from_inode_id = path_to_inode(argv[1]);
    if (from_inode_id == -1){ return EXIT_FAILURE;}
    struct inode from_inode = get_inode(from_inode_id);

    if (!from_inode.is_file){
        printf("%s is not a file\n", argv[1]);
        return EXIT_FAILURE;
    }

    if (contains_file(&to_dir_inode, to_file_name)){
        fprintf(stderr, "Directory or file \"%s\"with the same name already exists\n", argv[2]);
        return EXIT_FAILURE;
    }

    struct directory_item new_record = {0};
    new_record.inode = from_inode_id;
    strlcpy(new_record.item_name, to_file_name, sizeof(char[DIR_NAME_SIZE]));
    if (add_record_to_dir(new_record, &to_dir_inode)) { 
        printf("could not add file to directory\n");
        return EXIT_FAILURE;
    }
    
    // the source dir inode needs to be loaded now because the destination dir may be same as source dir,
    // changing it in file but not the struct in code
    struct inode from_dir_inode = get_inode(from_dir_id);
    delete_item(&from_dir_inode, from_file_name);

    return EXIT_SUCCESS;
}

int cmd_rm(int argc, char** argv) {
    // separate name of the file from path
    char* file_name = get_final_token(argv[1]); 
    if (!file_name || file_name[0] == '\0') {
        fprintf(stderr, "File name cannot be empty\n");
        return EXIT_FAILURE;
    }
 
    // get target directory
    int target_dir_id = path_to_dir_inode(argv[1]);
    if (target_dir_id == -1) return EXIT_FAILURE;
    struct inode target_dir = get_inode(target_dir_id);

    return delete_item(&target_dir, file_name);
}

int cmd_mkdir(int argc, char** argv) {
    
    // get parent directory and name of new one from args
    char* dir_name = get_final_token(argv[1]);
    
    if (!dir_name || dir_name[0] == '\0') {
        fprintf(stderr, "Directory name cannot be empty\n");
        return EXIT_FAILURE;
    }
    
    int paretn_dir_id = path_to_dir_inode(argv[1]);
    struct inode parent_inode = get_inode(paretn_dir_id);
    if (paretn_dir_id == -1) return EXIT_FAILURE;
    if (parent_inode.is_file) {
        printf("Path does not exist\n");
        return EXIT_FAILURE;
    }

    // check if the file name already is there
    if (contains_file(&parent_inode, dir_name)){
        fprintf(stderr, "Directory or file \"%s\" with the same name already exists\n", argv[1]);
        return EXIT_FAILURE;
    }

    // create new directory node
    int new_node_id = create_dir_node(paretn_dir_id); 

    // create new record to add
    struct directory_item dir_record = {0};
    dir_record.inode = new_node_id;
    strlcpy(dir_record.item_name, dir_name, sizeof(dir_record.item_name));

    add_record_to_dir(dir_record, &parent_inode);

    return EXIT_SUCCESS;
}

int cmd_rmdir(int argc, char** argv) {

    if (!strcmp(argv[1],".") || !strcmp(argv[1],"..")){
        printf("can not remove \".\" or \"..\" from directory");
        return EXIT_FAILURE;
    }

    // separate name of the dir from path using helper function
    char* dir_name = get_final_token(argv[1]);
    
    int target_node_id = path_to_dir_inode(argv[1]);
    struct inode target_inode = get_inode(target_node_id);
    delete_item(&target_inode, dir_name);

    return EXIT_SUCCESS;
}
    
int cmd_ls(int argc, char** argv) {

    struct inode curr_inode;
    if (argc == 2){
        int inode_id = path_to_inode(argv[1]);
        if (inode_id == -1) return EXIT_FAILURE;
        curr_inode = get_inode(inode_id);
    } else {
        curr_inode = get_inode(g_system_state.curr_node_id);     
    }

    struct directory_item* dir_content = get_directory_items(&curr_inode);
    if (!dir_content) return EXIT_FAILURE;
    int record_count = curr_inode.file_size / sizeof(struct directory_item);
    for (int i = 0; i < record_count; i++){
        struct inode item_inode = get_inode(dir_content[i].inode);
        const char* color = item_inode.is_file ? "" : "\033[34m";
        printf("%s%-12s\033[0m | inode: %3d | size: %6d bytes | refs: %d\n", 
               color,
               dir_content[i].item_name,
               dir_content[i].inode,
               item_inode.file_size,
               item_inode.references);
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
    if(path == NULL){ return EXIT_FAILURE;}
    printf("working directory: %s\n", path);
    free(path);
    return EXIT_SUCCESS;
}

int cmd_info(int argc, char** argv) {
    char* name = get_final_token(argv[1]);
    if (!name || name[0] == '\0') {
        fprintf(stderr, "Name cannot be empty\n");
        return EXIT_FAILURE;
    }
    int inode_id = path_to_inode(argv[1]);
    if (inode_id == -1){ return EXIT_FAILURE;}
    struct inode inode = get_inode(inode_id);
    int cluster_count = (inode.file_size + CLUSTER_SIZE -1) / CLUSTER_SIZE;
    int* clusters = get_node_clusters(&inode);

    const char* color = inode.is_file ? "" : "\033[34m";
    printf("%s%-12s\033[0m | inode: %4d | size: %6d bytes | refs: %2d", 
            color,
            name,
            inode.id,
            inode.file_size,
            inode.references);
    printf(" | clusters: [");
    for (int i = 0; i < cluster_count - 1; i++){
        printf("%d, ", clusters[1]);
    }
    printf("%d]\n", clusters[cluster_count - 1]);
    fflush(stdout);

    free(clusters);    
    return EXIT_SUCCESS;
}

int cmd_incp(int argc, char** argv) {

    FILE* fptr = fopen(argv[1],"r");

    if (!fptr){
        printf("File not found");
        return EXIT_FAILURE;
    }
    
    // separate name of the file from path using helper function
    char* file_name = get_final_token(argv[2]);
    
    if (!file_name || file_name[0] == '\0') {
        fprintf(stderr, "File name cannot be empty\n");
        fclose(fptr);
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

    // get node_id of the directory
    int target_dir_id = path_to_dir_inode(argv[2]);
    if(target_dir_id == -1){
        clear_inode(&inode);
        fclose(fptr);
        return EXIT_FAILURE;
    }

    // add the file into directory
    struct inode target_dir = get_inode(target_dir_id);
    struct directory_item item = {0};
    item.inode = inode.id;
    strlcpy(item.item_name, file_name, sizeof(item.item_name));
    add_record_to_dir(item, &target_dir);

    // Reload inode to get updated reference count and continue with file data
    inode = get_inode(new_node_id);

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
int ln(int argc, char** argv) {
    // get inode to link
    int original_inode_id = path_to_inode(argv[1]);
    if(original_inode_id == -1) return EXIT_FAILURE;
    struct inode original_node = get_inode(original_inode_id);
    if(!original_node.is_file){
        printf("can not hard link a directory");
        return EXIT_FAILURE;
    }

    // separate name of the file from path
    char* file_name = get_final_token(argv[2]); 
    if (!file_name || file_name[0] == '\0') {
        fprintf(stderr, "File name cannot be empty\n");
        return EXIT_FAILURE;
    }
 
    // get target directory
    int target_dir_id = path_to_dir_inode(argv[2]);
    if (target_dir_id == -1) return EXIT_FAILURE;
    struct inode target_dir = get_inode(target_dir_id);

    // check if file already exists
    if (contains_file(&target_dir, file_name)){
        fprintf(stderr, "Directory or file \"%s\"with the same name already exists\n", argv[2]);
        return EXIT_FAILURE;
    }

    // finally add the entry to directory
    struct directory_item record = {0};
    record.inode = original_inode_id;
    strlcpy(record.item_name, file_name, DIR_NAME_SIZE);
    add_record_to_dir(record, &target_dir);

    return EXIT_SUCCESS;
};

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
    {"ln", ln, 2},
    {"test", test, -1}
};

// Number of commands
const int NUM_COMMANDS = sizeof(commands) / sizeof(commands[0]);