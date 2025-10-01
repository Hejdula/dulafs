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
        
        // printf("length: %d\n", (int)strlen(input));


        // Check if input matches any command
        int commandFound = 0;
        for(int i = 0; i < NUM_COMMANDS; i++){
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