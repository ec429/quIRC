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
		*out=(char *)realloc(*out, ol+pl+4+strlen(CLR));
		x+=pl+1;
		if((x>=width) && (pl<width))
		{
			strcat(*out, "\n" CLR);
			ol++;
			x=pl;
		}
		else if(ptr!=msg)
		{
			strcat(*out, " ");
			ol++;
		}
		strcat(*out, ptr);
		ol+=pl;
		ptr=strtok(NULL, " ");
	}
	return(x);
}
