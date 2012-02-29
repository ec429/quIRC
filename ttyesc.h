#pragma once
/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-11 Edward Cree

	See quirc.c for license information
	ttyesc: terminfo-powered terminal escape sequences
*/

/*#define CLS			"\033[2J"		// CLear Screen
#define LOCATE		"\033[%d;%dH"	// Set cursor position (y,x)
#define CLR			"\033[K"		// CLear Line to right
#define CLA			"\033[2K"		// Clear Line All
#define SAVEPOS		"\033[s"		// Save cursor position
#define RESTPOS		"\033[u"		// Restore cursor position
*/
#include <stdio.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <term.h>
#include "bits.h"

int setcol(int fore, int back, bool hi, bool ul); // sets the text colour
int s_setcol(int fore, int back, bool hi, bool ul, char **rv, int *l, int *i); // writes a setcol-like string with append_char (see bits.h)
int resetcol(void); // default setcol() values
int s_resetcol(char **rv, int *l, int *i); // s_setcol to the colour set by resetcol
int settitle(const char *newtitle); // sets the window title if running in a term in a window system (eg. xterm)
int termsize(int fd, int *x, int *y);
