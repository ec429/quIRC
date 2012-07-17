#include <stdio.h>
#include <stdlib.h>
#include "ctbuf.h"

void ct_init_char(ctchar **buf, size_t *l, size_t *i)
{
	*l=80;
	*buf=malloc(*l*sizeof(ctchar));
	(*buf)[0].d=0;
	*i=0;
}

void ct_append_char(ctchar **buf, size_t *l, size_t *i, char d)
{
	ct_append_char_c(buf, l, i, (*buf&&*i)?(*buf)[(*i)-1].c:(colour){.fore=7,.back=0,.hi=false,.ul=false}, d);
}

void ct_append_char_c(ctchar **buf, size_t *l, size_t *i, colour c, char d)
{
	if(*buf)
	{
		(*buf)[*i]=(ctchar){.c=c, .d=d};
		(*i)++;
	}
	else
	{
		ct_init_char(buf, l, i);
		ct_append_char_c(buf, l, i, c, d);
	}
	ctchar *nbuf=*buf;
	if((*i)>=(*l))
	{
		*l=*i*2;
		nbuf=realloc(*buf, *l*sizeof(ctchar));
	}
	if(nbuf)
	{
		*buf=nbuf;
		(*buf)[*i].d=0;
	}
	else
	{
		free(*buf);
		ct_init_char(buf, l, i);
	}
}

void ct_append_str(ctchar **buf, size_t *l, size_t *i, const char *str)
{
	while(str && *str)
	{
		ct_append_char(buf, l, i, *str++);
	}
}

void ct_append_str_c(ctchar **buf, size_t *l, size_t *i, colour c, const char *str)
{
	while(str && *str)
	{
		ct_append_char_c(buf, l, i, c, *str++);
	}
}

void ct_putchar(ctchar a)
{
	setcolour(a.c);
	putchar(a.d);
}

void ct_puts(const ctchar *buf)
{
	if(!buf) return;
	size_t i=0;
	while(buf[i].d)
	{
		if(!(i&&eq_colour(buf[i].c, buf[i-1].c)))
			setcolour(buf[i].c);
		putchar(buf[i++].d);
	}
}

void ct_putsn(const ctchar *buf, size_t n)
{
	if(!buf) return;
	size_t i=0;
	while((buf[i].d)&&(i<n))
	{
		if(!(i&&eq_colour(buf[i].c, buf[i-1].c)))
			setcolour(buf[i].c);
		putchar(buf[i++].d);
	}
}
