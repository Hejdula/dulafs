#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "repl.h"
#include "commands.h"

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

    while(1){
        printf("dulafs:%s> ", g_systemState.current_dir);  // Show current directory in prompt
        fflush(stdout);
        
        // Safer fgets with error checking
        if (fgets(input, INPUT_BUFFER_SIZE, stdin) == NULL) {
            printf("\nError reading input or EOF reached\n");
            break;
        }
        
        input[strcspn(input, "\n")] = '\0'; // Remove newline character
        
        // Make a copy of input before strtok modifies it
        char* input_copy = strdup(input);
        // if (input_copy == NULL); 
        
        char* command = strtok(input_copy," ");
        if (command == NULL) {
            free(input_copy);
            continue;
        }

        // count tokens after first one
        int token_count = 1;
        while (strtok(NULL, " ") != NULL) token_count++;
        // printf("arg count: %d\n", token_count);

        input_copy = strdup(input);
        char** args = malloc(sizeof(char*) * token_count);

        args[0] = strtok(input_copy, " ");
        for(int i = 1; i < token_count; i++) {
            args[i] = strtok(NULL, " ");
            printf("arg[%d]: %s\n",i,args[i]);
        } 


        // Check if command matches any of our commands
        int command_found = 0;
        for(int i = 0; i < NUM_COMMANDS; i++){
            if(!strcmp(command, commands[i].name)){
                command_found = 1;
                commands[i].function(token_count,args); // Call function
                break;
            }
        }
        
        if(!command_found){
            printf("Unknown command: %s\n", command);
        }
        
        free(input_copy);
        free(args);
        if(!strcmp(command, "exit")){
            break;
        }
        
    }

    free(input);
    return;
}