/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	buffer: multiple-buffer control
*/

#include "buffer.h"

int init_buffer(int buf, btype type, char *bname, int nlines)
{
	bufs[buf].type=type;
	bufs[buf].bname=strdup(bname);
	bufs[buf].nlist=NULL;
	bufs[buf].handle=0;
	bufs[buf].server=0;
	bufs[buf].nick=NULL;
	bufs[buf].nlines=nlines;
	bufs[buf].ptr=0;
	bufs[buf].lc=(colour *)malloc(sizeof(colour[nlines]));
	bufs[buf].lt=(char **)malloc(sizeof(char *[nlines]));
	bufs[buf].ts=(time_t *)malloc(sizeof(time_t[nlines]));
	bufs[buf].filled=false;
	bufs[buf].alert=false;
	return(0);
}

int free_buffer(int buf)
{
	free(bufs[buf].bname);
	name *curr=bufs[buf].nlist;
	while(curr)
	{
		name *next=curr->next;
		free(curr->data);
		free(curr);
		curr=next;
	}
	bufs[buf].nlist=NULL;
	free(bufs[buf].lc);
	int l;
	for(l=0;l<(bufs[buf].filled?bufs[buf].nlines:bufs[buf].ptr);l++)
		free(bufs[buf].lt[l]);
	free(bufs[buf].lt);
	free(bufs[buf].ts);
	if(cbuf>=buf)
		cbuf--;
	nbufs--;
	int b;
	for(b=buf;b<nbufs;b++)
	{
		bufs[b]=bufs[b+1];
	}
	for(b=0;b<nbufs;b++)
	{
		if(bufs[b].server==buf)
		{
			bufs[b].server=0; // orphaned; should not happen
		}
		else if(bufs[b].server>buf)
		{
			bufs[b].server--;
		}
	}
	return(0);
}

int add_to_buffer(int buf, colour lc, char *lt)
{
	bufs[buf].lc[bufs[buf].ptr]=lc;
	if(bufs[buf].filled) free(bufs[buf].lt[bufs[buf].ptr]);
	bufs[buf].lt[bufs[buf].ptr]=strdup(lt);
	bufs[buf].ts[bufs[buf].ptr]=time(NULL);
	bufs[buf].ptr=(bufs[buf].ptr+1)%bufs[buf].nlines;
	if(bufs[buf].ptr==0)
		bufs[buf].filled=true;
	bufs[buf].alert=true;
	bufs[cbuf].alert=false;
	return(0);
}

int redraw_buffer(void)
{
	printf(LOCATE, height-1, 1);
	resetcol();
	char dash[width];
	memset(dash, '-', width-1);
	dash[width-1]=0;
	printf("%s" CLR "\n", dash);
	int l;
	for(l=(bufs[cbuf].filled?(bufs[cbuf].ptr+1)%bufs[cbuf].nlines:0);l!=bufs[cbuf].ptr;l=(l+1)%bufs[cbuf].nlines)
	{
		setcolour(bufs[cbuf].lc[l]);
		printf("%s" CLR "\n", bufs[cbuf].lt[l]);
		resetcol();
	}
	printf(CLA "\n");
	switch(bufs[cbuf].type)
	{
		case STATUS:
			settitle("quIRC - status");
		break;
		case SERVER: // have to scope it for the cstr 'variably modified type'
			{
				char cstr[16+strlen(bufs[cbuf].bname)];
				sprintf(cstr, "quIRC - %s", bufs[cbuf].bname);
				settitle(cstr);
			}
		break;
		case CHANNEL: // have to scope it for the cstr 'variably modified type'
			{
				char cstr[16+strlen(bufs[cbuf].bname)+strlen(bufs[bufs[cbuf].server].bname)];
				sprintf(cstr, "quIRC - %s on %s", bufs[cbuf].bname, bufs[bufs[cbuf].server].bname);
				settitle(cstr);
			}
		break;
		default:
			settitle("quIRC");
		break;
	}
	bufs[cbuf].alert=false;
	return(0);
}

int buf_print(int buf, colour lc, char *lt, bool nl)
{
	if(buf==cbuf)
	{
		setcolour(lc);
		if(nl) printf(CLA "\n");
		printf(LOCATE, height-2, 1);
		printf("%s" CLR "\n", lt);
		resetcol();
		printf(CLA "\n");
	}
	return(add_to_buffer(buf, lc, lt));
}

void in_update(char *inp)
{
	printf(LOCATE, height-1, 1);
	// tab strip
	int mbw = (width-1)/nbufs;
	int b;
	for(b=0;b<nbufs;b++)
	{
		putchar(' ');
		// (status) {server} [channel] <user>
		char brack[2]={'!', '!'};
		switch(bufs[b].type)
		{
			case STATUS:
				brack[0]='(';brack[1]=')';
			break;
			case SERVER:
				brack[0]='{';brack[1]='}';
			break;
			case CHANNEL:
				brack[0]='[';brack[1]=']';
			break;
			case PRIVATE:
				brack[0]='<';brack[1]='>';
			break;
		}
		colour c={7, 0, false, false};
		if(b==cbuf)
		{
			c.back=2;
			c.hi=true;
		}
		else if(b==bufs[cbuf].server)
		{
			c.back=4;
			c.ul=true;
		}
		if(bufs[b].alert)
		{
			c.fore=1;
			c.hi=true;
			c.ul=false; // can't have both at once: it's not really a bitmask
		}
		setcolour(c);
		putchar(brack[0]);
		if(strlen(bufs[b].bname)>mbw-3)
		{
			int r=(mbw-6)/2;
			printf("%.*s...%s", mbw-r-3, bufs[b].bname, bufs[b].bname+r);
		}
		else
		{
			printf("%s", bufs[b].bname);
		}
		putchar(brack[1]);
		resetcol();
	}
	printf(CLR "\n");
	// input
	int ino=inp?strlen(inp):0;
	if(ino>width-2)
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
