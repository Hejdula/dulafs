#ifndef COMMANDS_H
#define COMMANDS_H

// System state structure
struct System_State {
    char* current_dir;
};

// Command entry struct - combines name and function
struct CommandEntry {
    char* name;
    int (*function)(int argc,char** argv);
    int arg_count;
};

// Global system state declaration (defined in commands.c)
extern struct System_State g_systemState;

// Commands array declaration (defined in commands.c)
extern struct CommandEntry commands[];
extern const int NUM_COMMANDS;

#endif // COMMANDS_H