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

int wordline(char *msg, int x, char **out)
{
	off_t ol=(out&&*out)?strlen(*out)+1:0;
	char *ptr=strtok(msg, " ");
	while(ptr)
	{
		off_t pl=strlen(ptr);
		*out=(char *)realloc(*out, ol+pl+8+strlen(CLR));
		x+=pl+1;
		if((x>=width) && (pl<width))
		{
			strcat(*out, "\n" CLR);
			ol+=(1+strlen(CLR));
			x=pl;
		}
		else if(ptr!=msg)
		{
			strcat(*out, " ");
			ol++;
		}
		int optr=strlen(*out);
		while(*ptr)
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
						ol+=12;
						*out=(char *)realloc(*out, ol+pl+8+strlen(CLR));
						colour mcc=c_mirc(fore, back);
						(*out)[optr++]='\033';
						(*out)[optr++]='[';
						(*out)[optr++]='0';
						(*out)[optr++]=';';
						(*out)[optr++]='3';
						(*out)[optr++]='0'+mcc.fore;
						(*out)[optr++]=';';
						(*out)[optr++]='4';
						(*out)[optr++]='0'+mcc.back;
						(*out)[optr++]='m';
					}
				}
				else if(sscanf(ptr, "\003%u%zn", &fore, &bytes)==1)
				{
					ptr+=bytes;
					if(mirc_colour_compat==2)
					{
						ol+=12;
						*out=(char *)realloc(*out, ol+pl+8+strlen(CLR));
						colour mcc=c_mirc(fore, 1);
						(*out)[optr++]='\033';
						(*out)[optr++]='[';
						(*out)[optr++]='0';
						(*out)[optr++]=';';
						(*out)[optr++]='3';
						(*out)[optr++]='0'+mcc.fore;
						(*out)[optr++]=';';
						(*out)[optr++]='4';
						(*out)[optr++]='0'+mcc.back;
						(*out)[optr++]='m';
					}
				}
				else
				{
					ptr++;
					if(mirc_colour_compat==2)
					{
						ol+=12;
						*out=(char *)realloc(*out, ol+pl+8+strlen(CLR));
						(*out)[optr++]='\033';
						(*out)[optr++]='[';
						(*out)[optr++]='0';
						(*out)[optr++]=';';
						(*out)[optr++]='3';
						(*out)[optr++]='7';
						(*out)[optr++]=';';
						(*out)[optr++]='4';
						(*out)[optr++]='0';
						(*out)[optr++]='m';
					}
				}
			}
			else
			{
				(*out)[optr++]=*ptr++;
			}
		}
		(*out)[optr]=0;
		ol+=pl;
		ptr=strtok(NULL, " ");
	}
	return(x);
}

void append_char(char **buf, int *l, int *i, char c)
{
	(*buf)[(*i)++]=c;
	if((*i)>=(*l))
	{
		*l=*l*2;
		*buf=(char *)realloc(*buf, *l);
	}
	(*buf)[*i]=0;
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
