#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "repl.h"
#include "commands.h"
#include "dulafs.h"

#define INPUT_BUFFER_SIZE 1024

// Helper function to execute a command string
// Returns the error code from command execution
int execute_command_string(const char* input_string) {
    if (!input_string || input_string[0] == '\0') {
        return ERR_SUCCESS; // Empty lines are OK
    }
    
    char* input_copy = strdup(input_string);
    if (input_copy == NULL) { return ERR_MEMORY_ALLOCATION; }
    
    char* command_token = strtok(input_copy, " ");
    if (command_token == NULL) { 
        free(input_copy);
        return ERR_SUCCESS; // Empty or whitespace-only lines
    }

    // count tokens after first one
    int token_count = 1;
    while (strtok(NULL, " ") != NULL) token_count++;

    // Free the first copy and make a fresh one for argument parsing
    free(input_copy);
    input_copy = strdup(input_string);
    if (input_copy == NULL) { return ERR_MEMORY_ALLOCATION; }

    char** args = malloc(sizeof(char*) * token_count);
    if (args == NULL) { 
        free(input_copy);
        return ERR_MEMORY_ALLOCATION;
    }

    args[0] = strtok(input_copy, " ");
    char * command = args[0];
    for (int i = 1; i < token_count; i++) {
        args[i] = strtok(NULL, " ");
    }

    int error_code = ERR_UNKNOWN;
    
    // Check if command matches any known command and call its function
    for (int i = 0; i < NUM_COMMANDS; i++){
        if (!strcmp(command, commands[i].name)){
            if (
                !(token_count - 1 == commands[i].arg_count) 
                && commands[i].arg_count != -1
            ){
                error_code = ERR_INVALID_ARGC;
                break;
            }
            // execute the command
            error_code = commands[i].function(token_count, args);
            break;
        }
    }
    
    free(input_copy);
    free(args);
    return error_code;
}

void repl(){
    printf("Welcome to dula REPL, available commands: ");
    
    // Print available commands
    for(int i = 0; i < NUM_COMMANDS; i++){
        printf("%s", commands[i].name);
        if(i < NUM_COMMANDS - 1) printf(", ");
    }
    printf("\n");
    
    char* input = malloc(sizeof(char) * INPUT_BUFFER_SIZE);
    // char* argv;

    int last_error_num = 0;
    int last_command_executed = 0;

    while (1) {

        if (last_command_executed) {
            if (last_error_num == 0) { printf("[\033[0;32mok\033[0m] "); }
            else { printf("[\033[0;31m%d\033[0m] ", last_error_num); }
        }
        last_command_executed = 0;
        printf("\033[38;5;117mdulafs\033[0m:\033[38;5;227m%s\033[0m> ", g_system_state.working_dir);  // Show current directory in prompt (working dir is soft yellow)
        fflush(stdout); 

        // Safer fgets with error checking
        if (fgets(input, INPUT_BUFFER_SIZE, stdin) == NULL) {
            printf("\nError reading input\n");
            break;
        }
        
        input[strcspn(input, "\n")] = '\0'; // Remove newline character
        
        char* input_copy = strdup(input);
        if (input_copy == NULL) { printf("Memory allocation failed\n"); continue; }
        
        char* command_token = strtok(input_copy," ");
        if (command_token == NULL) { free(input_copy); continue; }

        if (!strcmp(command_token, "exit")){
            free(input_copy);
            break;
        }

        // count tokens after first one
        int token_count = 1;
        while (strtok(NULL, " ") != NULL) token_count++;

        // Free the first copy and make a fresh one for argument parsing
        free(input_copy);
        input_copy = strdup(input);
        if (input_copy == NULL) { printf("Memory allocation failed\n"); continue; }

        char** args = malloc(sizeof(char*) * token_count);
        if (args == NULL) { printf("Memory allocation failed\n"); free(input_copy); continue; }

        args[0] = strtok(input_copy, " ");
        char * command = args[0];
        for (int i = 1; i < token_count; i++) {
            args[i] = strtok(NULL, " ");
            // printf("arg[%d]: %s\n",i,args[i]);
        } 

        // Check if command matches any known command and call its function
        int command_found = 0;
        for (int i = 0; i < NUM_COMMANDS; i++){
            if (!strcmp(command, commands[i].name)){
                command_found = 1;
                if (
                    !(token_count - 1 == commands[i].arg_count) 
                    && commands[i].arg_count != -1
                ){
                    printf("Invalid number of arguments for function %s, expected: %d, got %d\n", commands[i].name, commands[i].arg_count, token_count - 1);
                    break;
                }
                // execute the command
                last_error_num = commands[i].function(token_count, args);
                last_command_executed = 1;
                if (last_error_num != ERR_SUCCESS) {
                    // Command failed, print error message
                    const char* error_msg = get_error_message((ErrorCode)last_error_num);
                    if (error_msg) {
                        fprintf(stderr, "Error: %s\n", error_msg);
                    } else {
                        fprintf(stderr, "Command returned with error code: %d\n", last_error_num);
                    }
                }
                break;
            }
        }
        
        if (!command_found){
            printf("Unknown command: %s\n", command);
        }
        
        free(input_copy);
        free(args);

    }

    free(input);
    return;
}