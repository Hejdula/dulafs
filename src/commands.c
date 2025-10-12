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
    char target_path[MAX_DIR_PATH];
    strcpy(target_path, argv[1]);

    // separate name of the dir from path
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
    delete_item(target_node_id, dir_name);

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

int cmd_cat(int argc, char** argv) { printf("TODO: Cat function called\n"); return 0; }

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
    {"test", test, -1},
};

// Number of commands
const int NUM_COMMANDS = sizeof(commands) / sizeof(commands[0]);