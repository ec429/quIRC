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

// helper fn macros
#define max(a,b)	((a)>(b)?(a):(b))
#define min(a,b)	((a)<(b)?(a):(b))

int width, height; // term size (set in quirc.c)

char * fgetl(FILE *); // gets a line of string data; returns a malloc-like pointer (preserves trailing \n)
int wordline(char *, int x, char **); // prepares a string for printing, breaking lines in between words
