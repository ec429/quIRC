/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	buffer: multiple-buffer control
*/

#include "buffer.h"

int init_buffer(buffer *buf, btype type, char *bname, int nlines)
{
	buf->type=type;
	buf->bname=bname;
	buf->nlist=NULL;
	buf->handle=0;
	buf->server=NULL;
	buf->nick=NULL;
	buf->nlines=nlines;
	buf->ptr=0;
	buf->lc=(colour *)malloc(nlines*sizeof(colour));
	buf->lt=(char **)malloc(nlines*sizeof(char *));
	buf->ts=(time_t *)malloc(nlines*sizeof(time_t));
	buf->filled=false;
	return(0);
}

int add_to_buffer(buffer *buf, colour lc, char *lt)
{
	buf->lc[buf->ptr]=lc;
	if(buf->filled) free(buf->lt[buf->ptr]);
	buf->lt[buf->ptr]=strdup(lt);
	buf->ts[buf->ptr]=time(NULL);
	buf->ptr=(buf->ptr+1)%buf->nlines;
	if(buf->ptr==0)
		buf->filled=true;
	return(0);
}

int buf_print(buffer *buf, colour lc, char *lt, bool nl)
{
	setcolour(lc);
	if(nl) printf(CLA "\n");
	printf(LOCATE, height-2, 1);
	printf("%s" CLR "\n" CLA "\n", lt);
	resetcol();
	if(buf)
		return(add_to_buffer(buf, lc, lt));
	return(0);
}

void in_update(char *inp)
{
	printf(LOCATE, height, 1);
	int ino=inp?strlen(inp):0;
	if(ino>78)
	{
		int off=20*max((ino+27-width)/20, 0);
		printf("%.10s ... %s" CLR, inp, inp+off+10);
	}
	else
	{
		printf("%s" CLR, inp?inp:"");
	}	
	fflush(stdout);
}
