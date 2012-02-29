/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-11 Edward Cree

	See quirc.c for license information
	ttyesc: terminfo-powered terminal escape sequences
*/

#include "ttyesc.h"

int setcol(int fore, int back, bool hi, bool ul)
{
	if((fore<0)||(fore>7))
		return(1);
	if((back<0)||(back>7))
		return(2);
	const char *sb=tigetstr("enter_bold_mode"), *nsb=tigetstr("exit_bold_mode");
	if(!sb) sb=tigetstr("enter_standout_mode");
	if(!sb) sb="";
	if(!nsb) nsb=tigetstr("exit_standout_mode");
	if(!nsb) nsb="";
	if(hi) puts(sb);
	else puts(nsb);
	const char *su=tigetstr("enter_underline_mode"), *nsu=tigetstr("exit_bold_mode");
	if(!su) su=tigetstr("enter_italics_mode");
	if(!su) su=tigetstr("enter_standout_mode");
	if(!su) su="";
	if(!nsu) nsu=tigetstr("exit_italics_mode");
	if(!nsu) nsu=tigetstr("exit_standout_mode");
	if(!nsu) nsu="";
	if(ul) puts(su);
	else puts(nsu);
	const char *f=tparm("set_foreground", fore);
	if(f) puts(f);
	const char *b=tparm("set_background", back);
	if(b) puts(b);
	fflush(stdout);
	return(0);
}

int s_setcol(int fore, int back, bool hi, bool ul, char **rv, int *l, int *i)
{
	if((fore<0)||(fore>7))
		return(1);
	if((back<0)||(back>7))
		return(2);
	const char *sb=tigetstr("enter_bold_mode"), *nsb=tigetstr("exit_bold_mode");
	if(!sb) sb=tigetstr("enter_standout_mode");
	if(!sb) sb="";
	if(!nsb) nsb=tigetstr("exit_standout_mode");
	if(!nsb) nsb="";
	if(hi) append_str(rv, l, i, sb);
	else append_str(rv, l, i, nsb);
	const char *su=tigetstr("enter_underline_mode"), *nsu=tigetstr("exit_bold_mode");
	if(!su) su=tigetstr("enter_italics_mode");
	if(!su) su=tigetstr("enter_standout_mode");
	if(!su) su="";
	if(!nsu) nsu=tigetstr("exit_italics_mode");
	if(!nsu) nsu=tigetstr("exit_standout_mode");
	if(!nsu) nsu="";
	if(ul) append_str(rv, l, i, su);
	else append_str(rv, l, i, nsu);
	const char *f=tparm("set_foreground", fore);
	if(f) append_str(rv, l, i, f);
	const char *b=tparm("set_background", back);
	if(b) append_str(rv, l, i, b);
	return(0);
}

int resetcol(void)
{
	return(setcol(7, 0, false, false));
}

int s_resetcol(char **rv, int *l, int *i)
{
	return(s_setcol(7, 0, false, false, rv, l, i));
}

int settitle(const char *newtitle)
{
	if(titles&&tigetflag("has_status_line"))
	{
		const char *tsl=tigetstr("to_status_line");
		const char *fsl=tigetstr("from_status_line");
		if(tsl&&fsl&&newtitle)
			printf("%s%s%s", tsl, newtitle, fsl);
		fflush(stdout);
	}
	return(0);
}

int termsize(int fd, int *x, int *y)
{
	struct winsize ws;
	if(ioctl(fd, TIOCGWINSZ, &ws)) return(1);
	if(x) *x=ws.ws_col;
	if(y) *y=ws.ws_row;
	return(0);
}
