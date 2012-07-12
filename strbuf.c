/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	strbuf: auto-reallocating string buffers
*/

#include <stdlib.h>
#include "strbuf.h"

void append_char(char **buf, size_t *l, size_t *i, char c)
{
	if(*buf)
	{
		(*buf)[(*i)++]=c;
	}
	else
	{
		init_char(buf, l, i);
		append_char(buf, l, i, c);
	}
	char *nbuf=*buf;
	if((*i)>=(*l))
	{
		*l=*i*2;
		nbuf=(char *)realloc(*buf, *l);
	}
	if(nbuf)
	{
		*buf=nbuf;
		(*buf)[*i]=0;
	}
	else
	{
		free(*buf);
		init_char(buf, l, i);
	}
}

void append_str(char **buf, size_t *l, size_t *i, const char *str)
{
	while(str && *str) // not the most tremendously efficient implementation, but conceptually simple at least
	{
		append_char(buf, l, i, *str++);
	}
}

void init_char(char **buf, size_t *l, size_t *i)
{
	*l=80;
	*buf=(char *)malloc(*l);
	(*buf)[0]=0;
	*i=0;
}

char * fgetl(FILE *fp)
{
	char * lout;
	size_t l,i;
	init_char(&lout, &l, &i);
	signed int c;
	while(!feof(fp))
	{
		c=fgetc(fp);
		if((c==EOF)||(c=='\n'))
			break;
		if(c!=0)
		{
			append_char(&lout, &l, &i, c);
		}
	}
	return(lout);
}

char *slurp(FILE *fp)
{
	char *fout;
	size_t l,i;
	init_char(&fout, &l, &i);
	signed int c;
	while(!feof(fp))
	{
		c=fgetc(fp);
		if(c==EOF)
			break;
		if(c!=0)
		{
			append_char(&fout, &l, &i, c);
		}
	}
	return(fout);
}
