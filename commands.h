#ifndef COMMANDS_H
#define COMMANDS_H

// System state structure
struct System_State {
    char* currentDir;
};

// Command entry struct - combines name and function
struct CommandEntry {
    char* name;
    void (*function)();
};

// Global system state declaration (defined in commands.c)
extern struct System_State g_systemState;

// Command function declarations
void cmd_format();
void cmd_cp();
void cmd_mv();
void cmd_rm();
void cmd_mkdir();
void cmd_rmdir();
void cmd_ls();
void cmd_cat();
void cmd_cd();
void cmd_pwd();
void cmd_info();
void cmd_incp();
void cmd_outcp();
void cmd_load();
void cmd_exit();
void cmd_statfs();

// Commands array declaration (defined in commands.c)
extern struct CommandEntry commands[];
extern const int NUM_COMMANDS;

#endif // COMMANDS_H