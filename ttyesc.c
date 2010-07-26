/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	ttyesc: ANSI Terminal Escape Sequences
*/

#include "ttyesc.h"

int setcol(int fore, int back, bool hi, bool ul)
{
	if(fore>7)
		return(1);
	if(back>7)
		return(2);
	if(fore>=0)
	{
		putchar('\033');
		putchar('[');
		putchar('0'+(hi?1:0)+(ul?4:0));
		putchar(';');
		putchar('3');
		putchar('0'+fore);
		putchar('m');
	}
	if(back>=0)
	{
		putchar('\033');
		putchar('[');
		putchar('0'+(hi?1:0)+(ul?4:0));
		putchar(';');
		putchar('4');
		putchar('0'+back);
		putchar('m');
	}
	fflush(stdout);
	return(0);
}

int resetcol(void)
{
	return(setcol(7, 0, false, false));
}

int settitle(char *newtitle)
{
	printf("\033]0;%s\007", newtitle);
	fflush(stdout);
	return(0);
}
