/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	bits: general helper functions
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int width, height; // term size (set in quirc.c)

char * fgetl(FILE *); // gets a line of string data; returns a malloc-like pointer (preserves trailing \n)
int wordline(char *, int x); // prints a string, breaking lines in between words
