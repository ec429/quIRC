#pragma once
/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	ttyesc: ANSI Terminal Escape Sequences
*/

#define CLS			"\033[2J"		// You might recognise these two names...
#define LOCATE		"\033[%d;%dH"	// as being very similar to some basic keywords... :p
#define CLR			"\033[K"		// CLear Line to right
#define CLA			"\033[2K"		// Clear Line All
#define SAVEPOS		"\033[s"		// Save cursor position
#define RESTPOS		"\033[u"		// Restore cursor position

#include <stdio.h>
#include <stdbool.h>
#include "bits.h"

int setcol(int fore, int back, bool hi, bool ul); // sets the text colour
int s_setcol(int fore, int back, bool hi, bool ul, char **rv, int *l, int *i); // writes a setcol-like string with append_char (see bits.h)
int resetcol(void); // default setcol() values
int settitle(char *newtitle); // sets the window title if running in a term in a window system (eg. xterm)
