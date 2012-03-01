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
	const char *sgr0=exit_attribute_mode;
	if(sgr0) putp(sgr0);
	const char *sb=enter_bold_mode;
	if(!sb) sb=enter_standout_mode;
	if(!sb) sb="";
	if(hi) putp(sb);
	const char *su=enter_underline_mode;
	if(!su) su=enter_italics_mode;
	if(!su) su=enter_standout_mode;
	if(!su) su="";
	if(ul) putp(su);
	const char *f=tparm(set_a_foreground, fore);
	if(!f) f=tparm(set_foreground, fore);
	if(f) putp(f);
	const char *b=tparm(set_a_background, back);
	if(!f) f=tparm(set_background, back);
	if(b) putp(b);
	fflush(stdout);
	return(0);
}

int s_setcol(int fore, int back, bool hi, bool ul, char **rv, int *l, int *i)
{
	if((fore<0)||(fore>7))
		return(1);
	if((back<0)||(back>7))
		return(2);
	const char *sgr0=exit_attribute_mode;
	if(sgr0) append_str(rv, l, i, sgr0);
	const char *sb=enter_bold_mode;
	if(!sb) sb=enter_standout_mode;
	if(!sb) sb="";
	if(hi) append_str(rv, l, i, sb);
	const char *su=enter_underline_mode;
	if(!su) su=enter_italics_mode;
	if(!su) su=enter_standout_mode;
	if(!su) su="";
	if(ul) append_str(rv, l, i, su);
	const char *f=tparm(set_a_foreground, fore);
	if(!f) f=tparm(set_foreground, fore);
	if(f) append_str(rv, l, i, f);
	const char *b=tparm(set_a_background, back);
	if(!f) f=tparm(set_background, back);
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

int cls(void)
{
	const char *cl=clear_screen;
	if(cl)
		putp(cl);
	else
	{
		int x,y;
		if(termsize(fileno(stdout), &x, &y))
		{
			x=80;y=25;
		}
		for(int dy=0;dy<y;dy++)
		{
			for(int dx=0;dx<x;x++)
				putchar(' ');
			if(!auto_right_margin)
				putchar('\n');
		}
	}
	return(0);
}

int s_cls(char **rv, int *l, int *i)
{
	const char *cl=clear_screen;
	if(cl)
		append_str(rv, l, i, cl);
	else
	{
		int x,y;
		if(termsize(fileno(stdout), &x, &y))
		{
			x=80;y=25;
		}
		for(int dy=0;dy<y;dy++)
		{
			for(int dx=0;dx<x;x++)
				append_char(rv, l, i, ' ');
			if(!auto_right_margin)
				append_char(rv, l, i, '\n');
		}
	}
	return(0);
}

int clr(void)
{
	const char *cl=clr_eol;
	if(cl)
		putp(cl);
	else
	{
		int x;
		if(termsize(fileno(stdout), &x, NULL))
			x=80;
		for(int dx=0;dx<x;x++)
			putchar(' ');
	}
	return(0);
}

int s_clr(char **rv, int *l, int *i)
{
	const char *cl=clr_eol;
	if(cl)
		append_str(rv, l, i, cl);
	else
	{
		int x;
		if(termsize(fileno(stdout), &x, NULL))
			x=80;
		for(int dx=0;dx<x;x++)
			append_char(rv, l, i, ' ');
	}
	return(0);
}

int locate(int y, int x)
{
	const char *loc=tparm(cursor_address, y-1, x-1);
	if(loc) putp(loc);
	else
	{
		const char *home=cursor_home, *down=cursor_down, *right=cursor_right;
		if(home&&down&&right)
		{
			putp(home);
			for(int dy=1;dy<y;dy++)
				putp(down);
			for(int dx=1;dx<x;dx++)
				putp(right);
		}
		else return(1);
	}
	return(0);
}

int s_locate(int y, int x, char **rv, int *l, int *i)
{
	const char *loc=tparm(cursor_address, y-1, x-1);
	if(loc) append_str(rv, l, i, loc);
	else
	{
		const char *home=cursor_home, *down=cursor_down, *right=cursor_right;
		if(home&&down&&right)
		{
			append_str(rv, l, i, home);
			for(int dy=1;dy<y;dy++)
				append_str(rv, l, i, down);
			for(int dx=1;dx<x;dx++)
				append_str(rv, l, i, right);
		}
		else return(1);
	}
	return(0);
}

int savepos(void)
{
	const char *sp=save_cursor;
	if(sp) putp(sp);
	return(sp!=NULL);
}

int restpos(void)
{
	const char *rp=restore_cursor;
	if(rp) putp(rp);
	return(rp!=NULL);
}

int settitle(const char *newtitle)
{
	if(titles&&has_status_line)
	{
		const char *tsl=to_status_line;
		const char *fsl=from_status_line;
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
