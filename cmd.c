
#include "buffer.h"
#include "cmd.h"

//cmd_funcs can't be malloc'd the regular way because of -Werror
//malloc returns a void pointer and with -Werror you can't
//cast from a regular pointer to a function pointer.
int (*cmd_funcs[NCMDS+1]) (char *cmd, char *args);
char *commands[NCMDS+1];

//functions defined farther down
static int __close(char *cmd, char *args);

void init_cmds()
{

	cmd_funcs[NCMDS - 1] = NULL;
	commands[NCMDS - 1] = NULL;

	START_ADDING_CMDS();
	ADD_CMD("close", __close);


}

int call_cmd(char *cmd, char *args)
{	int i = 0;
	char *ccmd = commands[0];
	
	while(ccmd){
		if(!strcmp(ccmd,cmd)){
			return cmd_funcs[i](cmd,args);
		}
		i++;
		ccmd++;
	}	
	return 1;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static int __close(char *cmd, char *args){
		switch(bufs[cbuf].type)
		{
			case STATUS:
				cmd="quit";
			break;
			case SERVER:
				if(bufs[cbuf].live)
				{
					cmd="disconnect";
				}
				else
				{
					free_buffer(cbuf);
					return(0);
				}
			break;
			case CHANNEL:
				if(bufs[cbuf].live)
				{
					cmd="part";
				}
				else
				{
					free_buffer(cbuf);
					return(0);
				}
			break;
			default:
				bufs[cbuf].live=false;
				free_buffer(cbuf);
				return(0);
			break;
		}
		return 1;
}
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

