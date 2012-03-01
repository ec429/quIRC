#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	bits: general helper functions (chiefly string manipulation)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ttyesc.h"
#include "colour.h"
#include "config.h"

// helper fn macros
#define max(a,b)	((a)>(b)?(a):(b))
#define min(a,b)	((a)<(b)?(a):(b))

#ifdef	NEED_STRNDUP
char *strndup(const char *s, size_t size);
#endif

char *fgetl(FILE *); // gets a line of string data; returns a malloc-like pointer
char *slurp(FILE *); // gets an entire file of string data; returns a malloc-like pointer
int wordline(const char *msg, unsigned int x, char **out, int *l, int *i, colour lc); // prepares a string for printing, breaking lines in between words; returns new x
void init_char(char **buf, int *l, int *i); // initialises a string buffer in heap.  *buf becomes a malloc-like pointer
void append_char(char **buf, int *l, int *i, char c); // adds a character to a string buffer in heap (and realloc()s if needed)
void append_str(char **buf, int *l, int *i, const char *str); // adds a string to a string buffer in heap (and realloc()s if needed)
void crush(char **buf, unsigned int len);
void scrush(char **buf, unsigned int len);
char *mktag(char *fmt, char *from);
