#include "bits.h"

char * fgetl(FILE *fp)
{
	char * lout = (char *)malloc(81);
	int i=0;
	signed int c;
	while(!feof(fp))
	{
		c=fgetc(fp);
		if(c==EOF) // EOF without '\n' - we'd better put an '\n' in
			c='\n';
		if(c!=0)
		{
			lout[i++]=c;
			if((i%80)==0)
			{
				if((lout=(char *)realloc(lout, i+81))==NULL)
				{
					printf("\nNot enough memory to store input!\n");
					free(lout);
					return(NULL);
				}
			}
		}
		if(c=='\n') // we do want to keep them this time
			break;
	}
	lout[i]=0;
	char *nlout=(char *)realloc(lout, i+1);
	if(nlout==NULL)
	{
		return(lout); // it doesn't really matter (assuming realloc is a decent implementation and hasn't nuked the original pointer), we'll just have to temporarily waste a bit of memory
	}
	return(nlout);
}

int wordline(char *msg, int x, char **out, colour lc)
{
	int tabx=x;
	if(tabx*2>width)
		tabx=8;
	int l,i,l2,i2;
	i=strlen(*out);
	l=i+1;
	char *word;
	char *ptr=msg;
	while(*ptr)
	{
		switch(*ptr)
		{
			case ' ':
				if(++x<width)
				{
					append_char(out, &l, &i, ' ');
				}
				else
				{
					append_char(out, &l, &i, '\n');
					for(x=0;x<tabx;x++)
						append_char(out, &l, &i, ' ');
				}
				ptr++;
			break;
			case '\n':
				if(*++ptr)
				{
					append_char(out, &l, &i, '\n');
					for(x=0;x<tabx;x++)
						append_char(out, &l, &i, ' ');
				}
			break;
			case '\t':
				while(x%8)
				{
					if(++x<width)
					{
						append_char(out, &l, &i, ' ');
					}
					else
					{
						append_char(out, &l, &i, '\n');
						for(x=0;x<tabx;x++)
							append_char(out, &l, &i, ' ');
						break;
					}
				}
				ptr++;
			break;
			default:
				init_char(&word, &l2, &i2);
				int wdlen=0;
				while(!strchr(" \t\n", *ptr))
				{
					if((*ptr==3) && mirc_colour_compat)
					{
						int fore=0, back=0;
						ssize_t bytes=0;
						if(sscanf(ptr, "\003%u,%u%zn", &fore, &back, &bytes)==2)
						{
							ptr+=bytes;
							if(mirc_colour_compat==2)
							{
								colour mcc=c_mirc(fore, back);
								s_setcol(mcc.fore, mcc.back, mcc.hi, mcc.ul, &word, &l2, &i2);
							}
						}
						else if(sscanf(ptr, "\003%u%zn", &fore, &bytes)==1)
						{
							ptr+=bytes;
							if(mirc_colour_compat==2)
							{
								colour mcc=c_mirc(fore, 1);
								s_setcol(mcc.fore, mcc.back, mcc.hi, mcc.ul, &word, &l2, &i2);
							}
						}
						else
						{
							ptr++;
							if(mirc_colour_compat==2)
							{
								s_setcol(lc.fore, lc.back, lc.hi, lc.ul, &word, &l2, &i2);
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
				if(wdlen+x<width)
				{
					append_str(out, &l, &i, word);
					x+=wdlen;
				}
				else
				{
					append_char(out, &l, &i, '\n');
					for(x=0;x<tabx;x++)
						append_char(out, &l, &i, ' ');
					append_str(out, &l, &i, word);
					x+=wdlen;
				}
			break;
		}
	}
	return(x);
}

void append_char(char **buf, int *l, int *i, char c)
{
	if(*buf)
	{
		(*buf)[(*i)++]=c;
	}
	else
	{
		init_char(buf, l, i);
	}
	char *nbuf=*buf;
	if((*i)>=(*l))
	{
		*l=*l*2;
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

void append_str(char **buf, int *l, int *i, char *str)
{
	while(*str) // not the most tremendously efficient implementation, but conceptually simple at least
	{
		append_char(buf, l, i, *str++);
	}
}

void init_char(char **buf, int *l, int *i)
{
	*l=80;
	*buf=(char *)malloc(*l);
	(*buf)[0]=0;
	*i=0;
}

void crush(char **buf, int len)
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

void scrush(char **buf, int len)
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
			*dot=0;
			crush(buf, len);
		}
	}
}
