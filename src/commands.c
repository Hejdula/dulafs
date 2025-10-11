#include <stdio.h>
#include "commands.h"
#include "dulafs.h"
#include "stdlib.h"
#include <string.h>

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
    int new_node_id = create_dir_node(g_system_state.curr_node_id); 
    struct directory_item dir_record = {0};
    dir_record.inode = new_node_id;
    strncpy(dir_record.item_name, argv[1], sizeof(dir_record.item_name) - 1);
    dir_record.item_name[sizeof(dir_record.item_name) - 1] = '\0';
    struct inode curr_node = get_inode(g_system_state.curr_node_id); 
    add_record_to_dir(dir_record, &curr_node);

    return EXIT_SUCCESS;
}

int cmd_rmdir(int argc, char** argv) { printf("TODO: Remove directory function called\n"); return 0; }

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

int cmd_cat(int argc, char** argv) { printf("TODO: Cat function called\n"); return 0; }

int cmd_cd(int argc, char** argv) {
    struct inode curr_inode = get_inode(g_system_state.curr_node_id);
    struct directory_item* dir_content = (struct directory_item*) get_node_data(&curr_inode);
    int record_count = curr_inode.file_size / sizeof(struct directory_item);
    int record_found = 0;
    for (int i = 0; i < record_count; i++){
        if (!strcmp(argv[1], dir_content[i].item_name)){
            struct inode found_node = get_inode(dir_content[i].inode);
            if (found_node.is_file){
                printf("%s is file, not a directory", argv[1]);
                free(dir_content);
                return EXIT_FAILURE;
            }
            record_found = 1;
            g_system_state.curr_node_id = dir_content[i].inode;
            break;
        }
    }    
    free(dir_content);
     
    if (!record_found){
        printf("No such directory found");
        return EXIT_FAILURE;
    }

    printf("current inode: %d", g_system_state.curr_node_id);


    return EXIT_SUCCESS;
}

int cmd_pwd(int argc, char** argv) {
    struct inode curr_inode = get_inode(g_system_state.curr_node_id);
    int prev_inode_id = -1;
    char path[MAX_DIR_PATH];
    char temp[MAX_DIR_PATH];
    path[0] = '\0';

    while(prev_inode_id != ROOT_NODE){
        struct directory_item* dir_content = (struct directory_item*) get_node_data(&curr_inode);

        if(prev_inode_id != -1){ 
            int record_found = 0;
            int record_count = curr_inode.file_size / sizeof(struct directory_item);

            //find name of the previous inode and prepend it to path with '/'
            for (int i = 0; i < record_count; i++){
                if (dir_content[i].inode == prev_inode_id){
                    struct inode found_node = get_inode(dir_content[i].inode);
                    strcpy(temp, "/");
                    strlcat(temp, dir_content[i].item_name, MAX_DIR_PATH);
                    strlcat(temp, path, MAX_DIR_PATH);
                    strlcpy(path, temp, MAX_DIR_PATH);
                    record_found = 1;
                    break;
                }
            }    

            if(!record_found){
                printf("path: %s\n", path);
                fprintf(stderr, "error: could not find inode in upper directory\n");
                free(dir_content);
                return EXIT_FAILURE;
            }
        }
        prev_inode_id = curr_inode.id;
        curr_inode = get_inode(dir_content[1].inode);
        free(dir_content);
        
    }
    printf("path: %s \n", path);

    return EXIT_SUCCESS;
}

int cmd_info(int argc, char** argv) { printf("TODO: Info function called\n"); return 0; }
int cmd_incp(int argc, char** argv) { printf("TODO: Input copy function called\n"); return 0; }
int cmd_outcp(int argc, char** argv) { printf("TODO: Output copy function called\n"); return 0; }
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
    {"cat", cmd_cat, 0},
    {"cd", cmd_cd, 1},
    {"pwd", cmd_pwd, 0},
    {"info", cmd_info, 0},
    {"incp", cmd_incp, 0},
    {"outcp", cmd_outcp, 0},
    {"load", cmd_load, 0},
    {"statfs", cmd_statfs, 0},
    {"test", test, 0},
};

// Number of commands
const int NUM_COMMANDS = sizeof(commands) / sizeof(commands[0]);