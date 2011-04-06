/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	ttyesc: ANSI Terminal Escape Sequences
*/

#include "ttyesc.h"

int setcol(int fore, int back, bool hi, bool ul)
{
	resetcol(); // reset it first
	if((fore<0)||(fore>7))
		return(1);
	if((back<0)||(back>7))
		return(2);
	putchar('\033');
	putchar('[');
	putchar('0'+(hi?1:0)+(ul?4:0));
	putchar(';');
	putchar('3');
	putchar('0'+fore);
	putchar(';');
	putchar('4');
	putchar('0'+back);
	putchar('m');
	fflush(stdout);
	return(0);
}

int s_setcol(int fore, int back, bool hi, bool ul, char **rv, int *l, int *i)
{
	if((fore<0)||(fore>7))
		return(1);
	if((back<0)||(back>7))
		return(2);
	append_char(rv, l, i, '\033');
	append_char(rv, l, i, '[');
	append_char(rv, l, i, '0'+(hi?1:0)+(ul?4:0));
	append_char(rv, l, i, ';');
	append_char(rv, l, i, '3');
	append_char(rv, l, i, '0'+fore);
	append_char(rv, l, i, ';');
	append_char(rv, l, i, '4');
	append_char(rv, l, i, '0'+back);
	append_char(rv, l, i, 'm');
	return(0);
}

int resetcol(void)
{
	printf("\033[0;37;40m");
	return(0);
}

int s_resetcol(char **rv, int *l, int *i)
{
	append_str(rv, l, i, "\033[0;37;40m");
	return(0);
}

int settitle(char *newtitle)
{
	printf("\033]0;%s\007", newtitle);
	fflush(stdout);
	return(0);
}
