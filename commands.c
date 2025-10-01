#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "commands.h"

// Global system state
struct System_State g_systemState = {
    .currentDir = "/"  // Initialize to root directory
};

// Command function implementations
void cmd_format() { printf("TODO: Format function called\n"); }
void cmd_cp() { printf("TODO: Copy function called\n"); }
void cmd_mv() { printf("TODO: Move function called\n"); }
void cmd_rm() { printf("TODO: Remove function called\n"); }
void cmd_mkdir() { printf("TODO: Make directory function called\n"); }
void cmd_rmdir() { printf("TODO: Remove directory function called\n"); }
void cmd_ls() { printf("TODO: List function called\n"); }
void cmd_cat() { printf("TODO: Cat function called\n"); }
void cmd_cd() { 
    printf("TODO: Change directory function called\n"); 
    // Example: g_systemState.currentDir = "/new/path";  // Can modify global state directly
}
void cmd_pwd() { printf("Current directory: %s\n", g_systemState.currentDir); }
void cmd_info() { printf("TODO: Info function called\n"); }
void cmd_incp() { printf("TODO: Input copy function called\n"); }
void cmd_outcp() { printf("TODO: Output copy function called\n"); }
void cmd_load() { printf("TODO: Load function called\n"); }
void cmd_exit() { printf("Exiting\n"); }
void cmd_statfs() { printf("TODO: Status filesystem function called\n"); }

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
    {"exit", cmd_exit}
};

// Number of commands (calculated at compile time)
const int NUM_COMMANDS = sizeof(commands) / sizeof(commands[0]);