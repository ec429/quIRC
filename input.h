#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	input: handle input routines
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

typedef struct
{
	int nlines;
	int ptr;
	int scroll;
	bool filled;
	char **line;
}
ibuffer;

#include "ttyesc.h"
#include "names.h"
#include "buffer.h"
#include "irc.h"

bool ttab;

int inputchar(char **inp, int *state);
char * slash_dequote(char *inp);
int cmd_handle(char *inp, char **qmsg, fd_set *master, int *fdmax);
void initibuf(ibuffer *i);
void addtoibuf(ibuffer *i, char *data);
void freeibuf(ibuffer *i);
