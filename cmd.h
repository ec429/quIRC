#pragma once
#include "buffer.h"
#include "names.h"

typedef int (*cmd_func)(char *cmd, char *args, char **qmsg, fd_set *master, int *fdmax, int flag);

struct cmd_t {
	cmd_func func;
	char *name;
	char *help;	
};

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

// Only singly-linked, lacks full information.
// Valid fields are ->data, ->next
extern name *cmds_as_nlist;
