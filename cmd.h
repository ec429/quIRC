#ifndef _CMD_H_
#define _CMD_H_



extern void (*cmd_funcs[])(char *cmd, char *args);
extern const char *commands[];

//Couldn't figure out how to get it into one macro.
//Figure this is better than having an apparently unused 
//variable in init_cmds(); -Russell
#define START_ADDING_CMDS() \
	int __i=0;


#define ADD_CMD(NAME,FUNCTION) \
	cmd_funcs[(__i)] = (FUNCTION);\
	commands[(__i)] = (NAME); \
	__i++;	\


void init_cmds();

//Commands
//Remember to increment NCMDS when adding commands. -Russell
#define NCMDS 1
void test(char *cmd, char *args);



#endif
