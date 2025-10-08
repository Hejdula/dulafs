#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "repl.h"
#include "commands.h"
#include "dulafs.h"

#define INPUT_BUFFER_SIZE 1024

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

    while (1) {

        printf("dulafs:%s> ", g_system_state.curr_dir);  // Show current directory in prompt
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
            printf("arg[%d]: %s\n",i,args[i]);
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
                    printf("invalid number of arguents for function %s, expected: %d, got %d\n", commands[i].name, commands[i].arg_count, token_count - 1);
                    continue;
                }
                // Call function
                int ret_val =  commands[i].function(token_count,args);
                if (!ret_val){
                    printf("function returned with success\n");
                } else {
                    printf("funciton returned with error\n");
                };
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