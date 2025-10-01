#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "repl.h"

#define INPUT_BUFFER_SIZE 1024

struct I_Node {
    int id;
};

struct System_State {
    char* currentDir;
};

// Global system state
struct System_State g_systemState = {
    .currentDir = "/"  // Initialize to root directory
};

// Command entry struct - combines name and function
struct CommandEntry {
    char* name;
    void (*function)();  // Pass arguments as string
};

// Command functions - each prints what it was called for (using global state)
void cmd_format() { printf("TODO: Format function called\n"); }
void cmd_cp() { printf("TODO: Copy function called\n"); }
void cmd_mv() { printf("TODO: Move function called\n"); }
void cmd_rm() { printf("TODO: Remove function called\n"); }
void cmd_mkdir() { printf("TODO: Make directory function called\n"); }
void cmd_rmdir() { printf("TODO: Remove directory function called\n"); }
void cmd_ls() { printf("TODO: List function called\n"); }
void cmd_cat() { printf("TODO: Cat function called\n"); }
void cmd_cd() { printf("TODO: Change directory function called\n"); }
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

void repl(){
    printf("Welcome to dula REPL, available commands: ");
    
    // Print available commands
    int numCommands = sizeof(commands) / sizeof(commands[0]);
    for(int i = 0; i < numCommands; i++){
        printf("%s", commands[i].name);
        if(i < numCommands - 1) printf(", ");
    }
    printf("\n");
    
    char* input = malloc(sizeof(char) * INPUT_BUFFER_SIZE);
    // char* argv;

    while(1){
        printf("dulafs:%s> ", g_systemState.currentDir);  // Show current directory in prompt
        fflush(stdout);
        
        // Safer fgets with error checking
        if (fgets(input, INPUT_BUFFER_SIZE, stdin) == NULL) {
            printf("\nError reading input or EOF reached\n");
            break;
        }
        
        input[strcspn(input, "\n")] = '\0'; // Remove newline character
        
        // printf("length: %d\n", (int)strlen(input));


        // Check if input matches any command
        int commandFound = 0;
        for(int i = 0; i < numCommands; i++){
            if(!strcmp(input, commands[i].name)){
                commandFound = 1;
                commands[i].function(); // Call function (no parameters needed with global state)
                break;
            }
        }
        
        if(!commandFound && strlen(input) > 0){
            printf("Unknown command: %s\n", input);
        }
        
        if(!strcmp(input,"exit")){
            break;
        }
    }

    free(input);
    return;
}