#pragma once
/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	ttyesc: terminfo-powered terminal escape sequences
*/

#include <stdio.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <term.h>
#undef tab
#include "bits.h"

int setcol(int fore, int back, bool hi, bool ul); // sets the text colour
int s_setcol(int fore, int back, bool hi, bool ul, char **rv, int *l, int *i); // writes a setcol-like string with append_char (see bits.h)
int resetcol(void); // default setcol() values
int s_resetcol(char **rv, int *l, int *i); // s_setcol to the colour set by resetcol
int cls(void); // CLear Screen
int s_cls(char **rv, int *l, int *i);
int clr(void); // Clear Line to Right
int s_clr(char **rv, int *l, int *i);
int locate(int y, int x); // Set cursor position
int s_locate(int y, int x, char **rv, int *l, int *i);
int savepos(void); // Save cursor position
int restpos(void); // Restore cursor position
int settitle(const char *newtitle); // sets the window title if running in a term in a window system (eg. xterm)
int termsize(int fd, int *x, int *y);
int termsgr0(void);
