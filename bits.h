#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	bits: general helper functions
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ttyesc.h"
#include "colour.h"
#include "config.h"

#define TAGLEN	(maxnlen+9) // size of buffer needed for mktag()

// helper fn macros
#define max(a,b)	((a)>(b)?(a):(b))
#define min(a,b)	((a)<(b)?(a):(b))

char * fgetl(FILE *); // gets a line of string data; returns a malloc-like pointer
int wordline(char *msg, int x, char **out, colour lc); // prepares a string for printing, breaking lines in between words
void init_char(char **buf, int *l, int *i); // initialises a string buffer in heap.  *buf becomes a malloc-like pointer
void append_char(char **buf, int *l, int *i, char c); // adds a character to a string buffer in heap (and realloc()s if needed)
void append_str(char **buf, int *l, int *i, char *str); // adds a string to a string buffer in heap (and realloc()s if needed)
void crush(char **buf, int len);
void scrush(char **buf, int len);
void mktag(char *buf, char *from, bool priv);
