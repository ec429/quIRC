#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	strbuf: auto-reallocating string buffers
*/

#include <stdio.h>

char *fgetl(FILE *); // gets a line of string data; returns a malloc-like pointer
char *slurp(FILE *); // gets an entire file of string data; returns a malloc-like pointer
void init_char(char **buf, int *l, int *i); // initialises a string buffer in heap.  *buf becomes a malloc-like pointer
void append_char(char **buf, int *l, int *i, char c); // adds a character to a string buffer in heap (and realloc()s if needed)
void append_str(char **buf, int *l, int *i, const char *str); // adds a string to a string buffer in heap (and realloc()s if needed)
