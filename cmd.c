
#include "buffer.h"
#include "cmd.h"

void (*cmd_funcs[])(char *cmd, char *args);
const char *commands[];

void init_cmds()
{
	int number_of_commands = 0;
	int i=0;

	((void *) cmd_funcs) =  malloc(number_of_commands * sizeof(void (*)(char *cmd, char *args)));
	commands = (const char *) malloc(number_of_commands * sizeof( const char * ));

	if(!cmd_funcs || !commands)
	{	
		fprintf(stderr, "Out of memory!\n");
		push_ring(&s_buf,QUIET);
		termsgr0();
		exit(1);
	}

	cmd_funcs[number_of_commands-1] = NULL;
	commands[number_of_commands-1] = NULL;

	ADD_CMD(i++,	"test",		test);

}

void test(char *cmd, char *args){

}
