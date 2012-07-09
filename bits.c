/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	bits: general helper functions (chiefly string manipulation)
*/

#include "bits.h"

int wordline(const char *msg, unsigned int x, char **out, int *l, int *i, colour lc)
{
	if(!msg) return(x);
	unsigned int tabx=x;
	if(tabx*2>width)
		tabx=8;
	int l2,i2;
	char *word;
	const char *ptr=msg;
	colour cc=lc; // current colour
	s_setcolour(cc, out, l, i);
	while(*ptr)
	{
		switch(*ptr)
		{
			case ' ':
				if(++x<width)
				{
					append_char(out, l, i, ' ');
				}
				else
				{
					append_char(out, l, i, '\n');
					s_setcolour(lc, out, l, i);
					for(x=0;x<tabx;x++)
						append_char(out, l, i, ' ');
					s_setcolour(cc, out, l, i);
				}
				ptr++;
			break;
			case '\n':
				if(*++ptr)
				{
					append_char(out, l, i, '\n');
					s_setcolour(lc, out, l, i);
					for(x=0;x<tabx;x++)
						append_char(out, l, i, ' ');
					s_setcolour(cc, out, l, i);
				}
			break;
			case '\t':;
				bool dx=true;
				while(dx||(x%8))
				{
					dx=false;
					if(++x<width)
					{
						append_char(out, l, i, ' ');
					}
					else
					{
						append_char(out, l, i, '\n');
						s_setcolour(lc, out, l, i);
						for(x=0;x<tabx;x++)
							append_char(out, l, i, ' ');
						s_setcolour(cc, out, l, i);
						break;
					}
				}
				ptr++;
			break;
			default:
				init_char(&word, &l2, &i2);
				colour oc=cc;
				int wdlen=0;
				while(!strchr(" \t\n", *ptr))
				{
					if((*ptr==3) && mirc_colour_compat)
					{
						int fore=0, back=0;
						ssize_t bytes=0;
						if(sscanf(ptr, "\003%2d,%2d%zn", &fore, &back, &bytes)==2)
						{
							ptr+=bytes;
							if(mirc_colour_compat==2)
							{
								colour mcc=c_mirc(fore, back);
								s_setcolour(cc=mcc, &word, &l2, &i2);
							}
						}
						else if(sscanf(ptr, "\003%2d%zn", &fore, &bytes)==1)
						{
							ptr+=bytes;
							if(mirc_colour_compat==2)
							{
								colour mcc=c_mirc(fore, 1);
								s_setcolour(cc=mcc, &word, &l2, &i2);
							}
						}
						else
						{
							ptr++;
							if(mirc_colour_compat==2)
							{
								s_setcolour(cc=lc, &word, &l2, &i2);
							}
						}
					}
					else
					{
						append_char(&word, &l2, &i2, *ptr++);
						wdlen++;
					}
					if(wdlen+tabx>=width)
					{
						ptr--;
						break;
					}
				}
				if(wdlen+x>=width)
				{
					append_char(out, l, i, '\n');
					s_setcolour(lc, out, l, i);
					for(x=0;x<tabx;x++)
						append_char(out, l, i, ' ');
					if(mirc_colour_compat==2)
						s_setcolour(oc, out, l, i);
					else
						s_setcolour(cc, out, l, i);
				}
				append_str(out, l, i, word);
				free(word);
				x+=wdlen;
			break;
		}
	}
	return(x);
}

void crush(char **buf, unsigned int len)
{
	if(strlen(*buf)>len)
	{
		char *b=*buf;
		if(*b=='~') b++;
		char *rv=(char *)malloc(len+1);
		if(strlen(b)>len)
		{
			int right=(len-1)/2;
			int left=(len-1)-right;
			sprintf(rv, "%.*s~%s", left, b, b+strlen(b)-right);
		}
		else
		{
			strcpy(rv, b);
		}
		free(*buf);
		*buf=rv;
	}
}

void scrush(char **buf, unsigned int len)
{
	if(strlen(*buf)>len)
	{
		if(strncmp(*buf, "irc.", 4)==0)
		{
			char *nb=(char *)malloc(strlen(*buf));
			nb[0]='~';
			strcpy(nb+1, (*buf)+4);
			free(*buf);
			scrush(&nb, len);
			*buf=nb;
		}
		else
		{
			char *dot=strchr(*buf, '.');
			char *n=dot;
			while(n)
			{
				dot=n;
				n=strchr(n+1, '.');
			}
			if(dot) *dot=0;
			crush(buf, len);
		}
	}
}

char *mktag(char *fmt, char *from)
{
	char *rv=NULL;
	if(strlen(from)<=maxnlen)
	{
		rv=(char *)malloc(strlen(fmt)+maxnlen);
		if(rv)
		{
			memset(rv, ' ', maxnlen+strlen(fmt)-1);
			sprintf(rv+maxnlen-strlen(from), fmt, from);
		}
	}
	return(rv);
}

#ifdef	NEED_STRNDUP
char *strndup(const char *s, size_t size)
{
	char *rv=(char *)malloc(size+1);
	if(rv==NULL) return(NULL);
	strncpy(rv, s, size);
	rv[size]=0;
	return(rv);
}
#endif
