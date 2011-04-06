#pragma once
/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-11 Edward Cree

	See quirc.c for license information
	colour: defined colours & mirc-colour-compat
*/

#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
	int fore;
	int back;
	bool hi;
	bool ul;
}
colour;

#include "ttyesc.h"

int setcolour(colour); // wrapper for setcol
int s_setcolour(colour, char **, int *, int *); // wrapper for s_setcol
colour c_mirc(int, int);
int c_init(void);

colour c_list[19];

// These should really be generated procedurally
#define c_msg		c_list
#define c_notice	(c_list+2)
#define c_join		(c_list+4)
#define c_part		(c_list+6)
#define c_quit		(c_list+8)
#define c_nick		(c_list+10)
#define c_actn		(c_list+12)
#define c_status	c_list[14]
#define c_err		c_list[15]
#define c_unk		c_list[16]
#define c_unn		c_list[17]
