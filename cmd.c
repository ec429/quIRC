
#include "buffer.h"
#include "cmd.h"

//cmd_funcs can't be malloc'd the regular way because of -Werror
//malloc returns a void pointer and with -Werror you can't
//cast from a regular pointer to a function pointer.
void (*cmd_funcs[NCMDS+1]) (char *cmd, char *args);
char *commands[NCMDS+1];

void init_cmds()
{

	cmd_funcs[NCMDS - 1] = NULL;
	commands[NCMDS - 1] = NULL;

	START_ADDING_CMDS();
	ADD_CMD("test", test);

}

void call_cmd(char *cmd, char *args)
{	int i = 0;
	char *ccmd = commands[0];
	
	while(ccmd){
		if(!strcmp(ccmd,cmd)){
			cmd_funcs[i](cmd,args);
		}
		i++;
		ccmd++;
	}	
}

void test(char *cmd, char *args)
{
	cmd = NULL;
	args = cmd;
	cmd = args;
}
