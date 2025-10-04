#include <stdio.h>
#ifndef COMMANDS_H
#define COMMANDS_H

// Command entry struct - combines name and function
struct CommandEntry {
    char* name;
    int (*function)(int argc,char** argv);
    int arg_count; // -1 if argument count can varry
};

// Commands array declaration (defined in commands.c)
extern struct CommandEntry commands[];
extern const int NUM_COMMANDS;

#endif // COMMANDS_H