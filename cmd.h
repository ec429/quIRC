#pragma once

extern int (*cmd_funcs[]) (char *cmd, char *args, char **qmsg, fd_set * master,
			   int *fdmax, int flag);
extern char *commands[];

//Couldn't figure out how to get it into one macro.
//Figure this is better than having an apparently unused 
//variable in init_cmds(); -Russell
#define START_ADDING_CMDS() \
	int __i=0;

#define ADD_CMD(NAME,FUNCTION) \
	cmd_funcs[(__i)] = (FUNCTION);\
	commands[(__i)] = (NAME); \
	__i++;	\


//macro so that we can easily change function arguments
//when we inevitably find out that we need more than we have.
//also going to be using this form a lot. -Russell
//flag is used to pass information between commands. see
//example in afk and nick.
#define CMD_FUN(NAME)\
	static int _handle_##NAME(char *cmd, char *args, char **qmsg, fd_set *master, int *fdmax, int flag)
#define CMD_FNAME(NAME)\
	_handle_##NAME

int init_cmds();
int call_cmd(char *cmd, char *args, char **qmsg, fd_set * master, int *fdmax);
