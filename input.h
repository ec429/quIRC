#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	input: handle input routines
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

typedef struct
{
	char *data;
	size_t l;
	size_t i;
}
ichar;

typedef struct
{
	ichar left;
	ichar right;
}
iline;

typedef struct
{
	int nlines;
	int ptr;
	int scroll;
	bool filled;
	char **line;
}
ibuffer;

#include "names.h"

bool ttab;

int inputchar(iline *inp, int *state);
char * slash_dequote(char *inp);
int cmd_handle(char *inp, char **qmsg, fd_set *master, int *fdmax);
void initibuf(ibuffer *i);
void addtoibuf(ibuffer *i, char *data);
void freeibuf(ibuffer *i);
char back_ichar(ichar *buf); // returns the deleted char
char front_ichar(ichar *buf); // returns the deleted char
void ifree(iline *buf);

void i_home(iline *inp);
void i_end(iline *inp);
