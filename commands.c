#include <stdio.h>
#include "commands.h"

// Global system state
struct System_State g_systemState = {
    .current_dir = "/"  // Initialize to root directory
};

// Command function implementations
void cmd_format(int argc, char** argv) { printf("TODO: Format function called\n"); }
void cmd_cp(int argc, char** argv) { printf("TODO: Copy function called\n"); }
void cmd_mv(int argc, char** argv) { printf("TODO: Move function called\n"); }
void cmd_rm(int argc, char** argv) { printf("TODO: Remove function called\n"); }
void cmd_mkdir(int argc, char** argv) { printf("TODO: Make directory function called\n"); }
void cmd_rmdir(int argc, char** argv) { printf("TODO: Remove directory function called\n"); }
void cmd_ls(int argc, char** argv) { printf("TODO: List function called\n"); }
void cmd_cat(int argc, char** argv) { printf("TODO: Cat function called\n"); }
void cmd_cd(int argc, char** argv) { printf("TODO: Change directory function called\n"); }
void cmd_pwd(int argc, char** argv) { printf("Current directory: %s\n", g_systemState.current_dir); }
void cmd_info(int argc, char** argv) { printf("TODO: Info function called\n"); }
void cmd_incp(int argc, char** argv) { printf("TODO: Input copy function called\n"); }
void cmd_outcp(int argc, char** argv) { printf("TODO: Output copy function called\n"); }
void cmd_load(int argc, char** argv) { printf("TODO: Load function called\n"); }
void cmd_statfs(int argc, char** argv) { printf("TODO: Status filesystem function called\n"); }

// Array of command structs - combines name and function in one place
struct CommandEntry commands[] = {
    {"format", cmd_format},
    {"cp", cmd_cp},
    {"mv", cmd_mv},
    {"rm", cmd_rm},
    {"mkdir", cmd_mkdir},
    {"rmdir", cmd_rmdir},
    {"ls", cmd_ls},
    {"cat", cmd_cat},
    {"cd", cmd_cd},
    {"pwd", cmd_pwd},
    {"info", cmd_info},
    {"incp", cmd_incp},
    {"outcp", cmd_outcp},
    {"load", cmd_load},
    {"statfs", cmd_statfs},
};

// Number of commands
const int NUM_COMMANDS = sizeof(commands) / sizeof(commands[0]);