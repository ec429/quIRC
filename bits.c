/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	bits: general helper functions (chiefly string manipulation)
*/

#include "bits.h"
#include "strbuf.h"
#include "ttyesc.h"
#include "config.h"

int wordline(const char *msg, unsigned int x, char **out, size_t *l, size_t *i, colour lc)
{
	if(!msg) return(x);
	unsigned int tabx=x;
	if(tabx*2>width)
		tabx=8;
	size_t l2,i2;
	char *word;
	const char *ptr=msg;
	colour cc=lc; // current colour
	bool bold=false, underline=false, reverse=false; // format statuses (for when mcc==2)
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
					unsigned int consumed=0;
					if(mirc_colour_compat)
					{
						int fore=0, back=0;
						ssize_t bytes=0;
						switch(*ptr)
						{
							case 2: // ^B -> bold
								consumed=1;
								if(mirc_colour_compat==2)
								{
									cc.hi=(bold=!bold)||lc.hi;
									s_setcolour(cc, &word, &l2, &i2);
								}
							break;
							case 3: // ^C -> text colour
								if(sscanf(ptr, "\003%2d,%2d%zn", &fore, &back, &bytes)==2) // ^CNN,MM -> fore=N, back=M
								{
									consumed=bytes;
									if(mirc_colour_compat==2)
									{
										cc=reverse_colours(c_mirc(fore, back), reverse);
										cc.hi=bold||lc.hi;
										cc.ul=underline||lc.ul;
										s_setcolour(cc, &word, &l2, &i2);
									}
								}
								else if(sscanf(ptr, "\003%2d%zn", &fore, &bytes)==1) // ^CNN -> fore=N, back=default
								{
									consumed=bytes;
									if(mirc_colour_compat==2)
									{
										cc=c_mirc(fore, 1);
										cc.back=lc.back;
										cc=reverse_colours(cc, reverse);
										cc.hi=bold||lc.hi;
										cc.ul=underline||lc.ul;
										s_setcolour(cc, &word, &l2, &i2);
									}
								}
								else // ^C -> clear colours
								{
									consumed=1;
									if(mirc_colour_compat==2)
									{
										cc=reverse_colours(lc, reverse);
										cc.hi=bold||lc.hi;
										cc.ul=underline||lc.ul;
										s_setcolour(cc, &word, &l2, &i2);
									}
								}
							break;
							case 15: // ^O -> ordinary, clear all attributes
								consumed=1;
								if(mirc_colour_compat==2)
								{
									bold=underline=reverse=false;
									s_setcolour(cc=lc, &word, &l2, &i2);
								}
							break;
							case 18: // ^R -> reverse video
								consumed=1;
								if(mirc_colour_compat==2)
								{
									reverse=!reverse;
									s_setcolour(cc=reverse_colours(cc, true), &word, &l2, &i2);
								}
							break;
							case 21: // ^U -> underline
								consumed=1;
								if(mirc_colour_compat==2)
								{
									cc.ul=(underline=!underline)||lc.ul;
									s_setcolour(cc, &word, &l2, &i2);
								}
							break;
							default:
							break;
						}
					}
					if(!consumed)
					{
						if(*(unsigned char *)ptr<0x20) // TODO add UTF-8 decoding and replace this with isprint
						{
							consumed=1;
							char buf[16];
							snprintf(buf, 16, "\\%03o", *(unsigned char *)ptr);
							append_str(&word, &l2, &i2, buf);
							wdlen+=strlen(buf);
						}
					}
					ptr+=consumed;
					if(!consumed)
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
		size_t n=strlen(fmt)+maxnlen;
		rv=malloc(n);
		if(rv)
		{
			memset(rv, ' ', maxnlen+strlen(fmt)-1);
			ssize_t off=maxnlen-strlen(from);
			snprintf(rv+off, n-off, fmt, from);
		}
	}
	else
	{
		size_t n=strlen(fmt)+strlen(from);
		rv=malloc(n);
		if(rv)
			snprintf(rv, n, fmt, from);
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
