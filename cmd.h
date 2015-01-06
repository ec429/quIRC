#ifndef _CMD_H_
#define _CMD_H_



extern void (*cmd_funcs[])(char *cmd, char *args);
extern const char *commands[];


#define ADD_CMD(INDEX,NAME,FUNCTION) \
{ \
	cmd_funcs[(INDEX)] = (FUNCTION);\
	commands[(INDEX)] = (NAME); \
}



void init_cmds();


void test(char *cmd, char *args);



#endif
