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
        printf("dulafs:%s> ", g_systemState.currentDir);  // Show current directory in prompt
        fflush(stdout);
        
        // Safer fgets with error checking
        if (fgets(input, INPUT_BUFFER_SIZE, stdin) == NULL) {
            printf("\nError reading input or EOF reached\n");
            break;
        }
        
        input[strcspn(input, "\n")] = '\0'; // Remove newline character
        
        // Make a copy of input before strtok modifies it
        char* inputCopy = strdup(input);
        // if (inputCopy == NULL); 
        
        char* command = strtok(inputCopy," ");
        if (command == NULL) continue; 

        // count tokens after first one
        int tokenCount = 1;
        while (strtok(NULL, " ") != NULL) tokenCount++;
        // printf("arg count: %d\n", tokenCount);

        inputCopy = strdup(input);
        char** args = malloc(sizeof(char*) * tokenCount);

        args[0] = strtok(inputCopy, " ");
        for(int i = 1; i < tokenCount; i++) {
            args[i] = strtok(NULL, " ");
            printf("arg[%d]: %s\n",i,args[i]);
        } 


        // Check if command matches any of our commands
        int commandFound = 0;
        for(int i = 0; i < NUM_COMMANDS; i++){
            if(!strcmp(command, commands[i].name)){
                commandFound = 1;
                commands[i].function(tokenCount,args); // Call function
                break;
            }
        }
        
        if(!commandFound){
            printf("Unknown command: %s\n", command);
        }
        
        free(inputCopy);
        free(args);
        if(!strcmp(command, "exit")){
            break;
        }
        
    }

    free(input);
    return;
}