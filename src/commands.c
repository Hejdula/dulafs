#include <stdio.h>
#include <string.h>
#include "commands.h"
#include "dulafs.c"

// Command function implementations
int cmd_format(int argc, char** argv) {

    printf("Format function called\n"); 
    if(argc != 2){
        return 1;
    }

    return format(1000000);
}
int cmd_cp(int argc, char** argv) { printf("TODO: Copy function called\n"); return 0; }
int cmd_mv(int argc, char** argv) { printf("TODO: Move function called\n"); return 0; }
int cmd_rm(int argc, char** argv) { printf("TODO: Remove function called\n"); return 0; }
int cmd_mkdir(int argc, char** argv) { printf("TODO: Make directory function called\n"); return 0; }
int cmd_rmdir(int argc, char** argv) { printf("TODO: Remove directory function called\n"); return 0; }
int cmd_ls(int argc, char** argv) { printf("TODO: List function called\n"); return 0; }
int cmd_cat(int argc, char** argv) { printf("TODO: Cat function called\n"); return 0; }
int cmd_cd(int argc, char** argv) { printf("TODO: Change directory function called\n"); return 0; }
int cmd_pwd(int argc, char** argv) { printf("Current directory: %s\n", g_system_state.current_dir); return 0; }
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
    {"cd", cmd_cd, 0},
    {"pwd", cmd_pwd, 0},
    {"info", cmd_info, 0},
    {"incp", cmd_incp, 0},
    {"outcp", cmd_outcp, 0},
    {"load", cmd_load, 0},
    {"statfs", cmd_statfs, 0},
};

// Number of commands
const int NUM_COMMANDS = sizeof(commands) / sizeof(commands[0]);