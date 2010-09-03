/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	buffer: multiple-buffer control
*/

#include "buffer.h"

int initialise_buffers(int buflines, char *nick)
{
	bufs=(buffer *)malloc(sizeof(buffer));
	if(!bufs)
		return(1);
	init_buffer(0, STATUS, "status", buflines); // buf 0 is always STATUS
	nbufs=1;
	cbuf=0;
	bufs[0].live=true; // STATUS is never dead
	bufs[0].nick=nick;
	buf_print(0, c_status, GPL_MSG); // can't w_buf_print() it because it has embedded newlines
	return(0);
}

int init_buffer(int buf, btype type, char *bname, int nlines)
{
	bufs[buf].type=type;
	bufs[buf].bname=strdup(bname);
	bufs[buf].nlist=NULL;
	bufs[buf].ilist=NULL;
	bufs[buf].handle=0;
	bufs[buf].server=0;
	bufs[buf].nick=NULL;
	bufs[buf].topic=NULL;
	bufs[buf].nlines=nlines;
	bufs[buf].ptr=0;
	bufs[buf].scroll=0;
	bufs[buf].lc=(colour *)malloc(sizeof(colour[nlines]));
	bufs[buf].lt=(char **)malloc(sizeof(char *[nlines]));
	int i;
	for(i=0;i<bufs[buf].nlines;i++)
	{
		bufs[buf].lt[i]=malloc(1); // free() safe
	}
	bufs[buf].ts=(time_t *)malloc(sizeof(time_t[nlines]));
	bufs[buf].filled=false;
	bufs[buf].alert=false;
	bufs[buf].namreply=false;
	bufs[buf].live=false;
	bufs[buf].casemapping=RFC1459;
	return(0);
}

int free_buffer(int buf)
{
	if(bufs[buf].live)
	{
		w_buf_print(buf, c_err, "Buffer is still live!", "free_buffer:");
		return(1);
	}
	else
	{
		free(bufs[buf].bname);
		n_free(bufs[buf].nlist);
		bufs[buf].nlist=NULL;
		n_free(bufs[buf].ilist);
		bufs[buf].ilist=NULL;
		free(bufs[buf].lc);
		if(bufs[buf].topic) free(bufs[buf].topic);
		int l;
		for(l=0;l<bufs[buf].nlines;l++)
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
				bufs[b].live=false;
			}
			else if(bufs[b].server>buf)
			{
				bufs[b].server--;
			}
		}
		return(0);
	}
}

int add_to_buffer(int buf, colour lc, char *lt)
{
	if(buf>=nbufs)
	{
		w_buf_print(0, c_err, "Line was written to bad buffer!  Contents below.", "add_to_buffer()");
		w_buf_print(0, lc, lt, "");
		return(1);
	}
	char *nl;
	while((nl=strchr(lt, '\n'))!=NULL)
	{
		char *ln=strndup(lt, (size_t)(nl-lt));
		add_to_buffer(buf, lc, ln);
		free(ln);
		lt=nl+1;
	}
	bufs[buf].lc[bufs[buf].ptr]=lc;
	free(bufs[buf].lt[bufs[buf].ptr]);
	bufs[buf].lt[bufs[buf].ptr]=strdup(lt);
	bufs[buf].ts[bufs[buf].ptr]=time(NULL);
	bufs[buf].ptr=(bufs[buf].ptr+1)%bufs[buf].nlines;
	if(bufs[buf].ptr==0)
		bufs[buf].filled=true;
	if(bufs[buf].scroll)
		bufs[buf].scroll=min(bufs[buf].scroll+1, bufs[buf].nlines-1);
	bufs[buf].alert=true;
	bufs[cbuf].alert=false;
	return(0);
}

int redraw_buffer(void)
{
	int sl = ( bufs[cbuf].filled ? (bufs[cbuf].ptr+max(bufs[cbuf].nlines-(bufs[cbuf].scroll+height-3), 1))%bufs[cbuf].nlines : max(bufs[cbuf].ptr-(bufs[cbuf].scroll+height-3), 0) );
	int el = ( bufs[cbuf].filled ? (bufs[cbuf].ptr+bufs[cbuf].nlines-bufs[cbuf].scroll)%bufs[cbuf].nlines : max(bufs[cbuf].ptr-bufs[cbuf].scroll, 0) );
	int dl=el-sl;
	if(dl<0) dl+=bufs[cbuf].nlines;
	printf(CLS LOCATE, height-(dl+1), 1);
	resetcol();
	int l;
	for(l=sl;l!=el;l=(l+1)%bufs[cbuf].nlines)
	{
		setcolour(bufs[cbuf].lc[l]);
		if(full_width_colour)
		{
			printf("%s" CLR, bufs[cbuf].lt[l]);
			resetcol();
			printf("\n");
		}
		else
		{
			printf("%s", bufs[cbuf].lt[l]);
			resetcol();
			printf(CLR "\n");
		}
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
	if(tsb)
		titlebar();
	bufs[cbuf].alert=false;
	return(0);
}

int buf_print(int buf, colour lc, char *lt)
{
	if(force_redraw)
	{
		int e=add_to_buffer(buf, lc, lt);
		redraw_buffer();
		return(e);
	}
	if((buf==cbuf) && (bufs[buf].scroll==0))
	{
		setcolour(lc);
		printf(CLA "\n");
		printf(LOCATE CLA, height-1, 1);
		printf(LOCATE, height-2, 1);
		if(full_width_colour)
		{
			printf(CLA "%s" CLR, lt);
			resetcol();
			printf("\n" CLA "\n");
		}
		else
		{
			printf(CLA "%s", lt);
			resetcol();
			printf(CLR "\n" CLA "\n");
		}
		if(tsb)
			titlebar();
	}
	return(add_to_buffer(buf, lc, lt));
}

void in_update(char *inp)
{
	height=max(height, 5); // anything less than this is /silly/
	width=max(width, 30); // widths less than 30 break things, and are /also silly/
	resetcol();
	printf(LOCATE, height-1, 1);
	// tab strip
	int mbw = (width-1)/nbufs;
	int b;
	for(b=0;b<nbufs;b++)
	{
		colour c={7, hilite_tabstrip?5:0, false, false};
		setcolour(c);
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
		if(!LIVE(b))
		{
			c.fore=3;
			c.hi=true;
			c.ul=false; // can't have both at once: it's not really a bitmask
		}
		setcolour(c);
		putchar(brack[0]);
		char *tab=strdup(bufs[b].bname);
		if(bufs[b].type==SERVER)
		{
			scrush(&tab, mbw-3);
		}
		else
		{
			crush(&tab, mbw-3);
		}
		printf("%s", tab);
		free(tab);
		putchar(brack[1]);
		c.fore=7;
		c.back=hilite_tabstrip?5:0;
		c.hi=c.ul=false;
		setcolour(c);
	}
	printf(CLR);
	resetcol();
	printf("\n");
	// input
	int ino=inp?strlen(inp):0;
	if(ino>width-2)
	{
		int shw=max(min(10, width-27), 0);
		int skip=(width>=40)?20:10;
		int off=0;
		while(strlen(inp+off+shw)>width-(6+shw))
			off+=skip;
		char l[11];
		sprintf(l, "%.*s", shw, inp);
		char *lh=highlight(l);
		char *rh=highlight(inp+off+shw);
		printf("%s ... %s" CLR, lh, rh);
		free(lh);
		free(rh);
	}
	else
	{
		char *h=highlight(inp?inp:"");
		printf("%s" CLR, h);
		free(h);
	}	
	fflush(stdout);
}

char *highlight(char *src)
{
	int l,i;char *rv;
	init_char(&rv, &l, &i);
	while(*src)
	{
		if(*src=='\\')
		{
			switch(src[1])
			{
				case 'n':
					s_setcol(7, 0, 1, 0, &rv, &l, &i);
					append_char(&rv, &l, &i, '\\');
					append_char(&rv, &l, &i, 'n');
					s_setcol(7, 0, 0, 0, &rv, &l, &i);
					src++;
				break;
				case 'r':
					s_setcol(7, 0, 1, 0, &rv, &l, &i);
					append_char(&rv, &l, &i, '\\');
					append_char(&rv, &l, &i, 'r');
					s_setcol(7, 0, 0, 0, &rv, &l, &i);
					src++;
				break;
				case '\\':
					s_setcol(3, 0, 1, 0, &rv, &l, &i);
					append_char(&rv, &l, &i, '\\');
					append_char(&rv, &l, &i, '\\');
					s_setcol(7, 0, 0, 0, &rv, &l, &i);
					src++;
				break;
				case 0:
					s_setcol(4, 0, 1, 0, &rv, &l, &i);
					append_char(&rv, &l, &i, '\\');
					s_setcol(7, 0, 0, 0, &rv, &l, &i);
				break;
				case '0':
				case '1':
				case '2':
				case '3':
					s_setcol(6, 0, 1, 0, &rv, &l, &i);
					append_char(&rv, &l, &i, '\\');
					append_char(&rv, &l, &i, *++src);
					int digits=0;
					while(isdigit(src[1])&&(src[1]<'8')&&(++digits<3))
					{
						append_char(&rv, &l, &i, *++src);
					}
					s_setcol(7, 0, 0, 0, &rv, &l, &i);
				break;
				default:
					s_setcol(1, 0, 1, 0, &rv, &l, &i);
					append_char(&rv, &l, &i, '\\');
					s_setcol(7, 0, 0, 0, &rv, &l, &i);
				break;
			}
		}
		else
		{
			append_char(&rv, &l, &i, *src);
		}
		src++;
	}
	return(rv);
}

int w_buf_print(int buf, colour lc, char *lt, char *lead)
{
	char *ltd=strdup(lt);
	char *out=strdup(lead);
	wordline(ltd, strlen(out), &out, lc);
	int e=buf_print(buf, lc, out);
	free(ltd);
	free(out);
	return(e);
}

void titlebar(void)
{
	printf(LOCATE CLA, 1, 1);
	setcol(0, 7, true, false);
	int gits;
	sscanf(VERSION_TXT, "%u", &gits);
	const char *hashgit=strchr(VERSION_TXT, ' ');
	if(hashgit)
		hashgit++;
	char *cserv=strdup(bufs[bufs[cbuf].server].bname?bufs[bufs[cbuf].server].bname:"");
	char *cnick=strdup(bufs[bufs[cbuf].server].nick?bufs[bufs[cbuf].server].nick:"");
	char *cchan=(bufs[cbuf].type==CHANNEL)?strdup(bufs[cbuf].bname?bufs[cbuf].bname:""):strdup("");
	char *topic=(bufs[cbuf].type==CHANNEL)?bufs[cbuf].topic:NULL;
	scrush(&cserv, 16);
	crush(&cnick, 16);
	crush(&cchan, 16);
	char tbar[width];
	int wleft=width-1;
	bool use[8]; // #chan	nick	quIRC	version	gits	ghashgit	server	topic...
	char version[32];
	sprintf(version, "%hhu.%hhu.%hhu", VERSION_MAJ, VERSION_MIN, VERSION_REV);
	char vgits[8];
	sprintf(vgits, "%hhu", gits);
	memset(use, 0, sizeof(bool[8]));
	if(*cchan && (wleft>=strlen(cchan)+1))
	{
		use[0]=true;
		wleft-=strlen(cchan)+1;
	}
	if(*cnick && (wleft>=strlen(cnick)+1))
	{
		use[1]=true;
		wleft-=strlen(cnick)+1;
	}
	if(wleft>=6)
	{
		use[2]=true;
		wleft-=6;
	}
	if(wleft>=strlen(version)+1)
	{
		use[3]=true;
		wleft-=strlen(version)+1;
	}
	if(VERSION_TXT[0] && gits && use[3] && (wleft>=strlen(vgits)+1))
	{
		use[4]=true;
		wleft-=strlen(vgits)+1;
	}
	if(hashgit && use[4] && (wleft>=strlen(hashgit)+1))
	{
		use[5]=true;
		wleft-=strlen(hashgit)+1;
	}
	if(*cserv && (wleft>=strlen(cserv)+1))
	{
		use[6]=true;
		wleft-=strlen(cserv)+1;
	}
	else if(*cserv && wleft>1)
	{
		use[6]=true;
		scrush(&cserv, wleft-1);
		wleft-=strlen(cserv)+1;
	}
	if(topic && (wleft>1))
	{
		use[7]=true;
	}
	int i;
	for(i=0;i<width;i++)
		tbar[i]='-';
	tbar[width]=0;
	int p=1;
	// 2quIRC 3version 4gits 5ghashgit 6server 0chan 1nick 7topic
	// 0#chan	1nick	2quIRC	3version	4gits	5ghashgit	6server	7topic...
	if(use[2])
	{
		memcpy(tbar+p, "quIRC", 5);
		p+=6;
	}
	if(use[3])
	{
		memcpy(tbar+p, version, strlen(version));
		p+=strlen(version)+1;
	}
	if(use[4])
	{
		memcpy(tbar+p, vgits, strlen(vgits));
		p+=strlen(vgits)+1;
	}
	if(use[5])
	{
		memcpy(tbar+p, hashgit, strlen(hashgit));
		p+=strlen(hashgit)+1;
	}
	if(use[6])
	{
		memcpy(tbar+p, cserv, strlen(cserv));
		p+=strlen(cserv)+1;
	}
	if(use[0])
	{
		memcpy(tbar+p, cchan, strlen(cchan));
		p+=strlen(cchan)+1;
	}
	if(use[1])
	{
		memcpy(tbar+p, cnick, strlen(cnick));
		p+=strlen(cnick)+1;
	}
	if(use[7])
	{
		memcpy(tbar+p, topic, min(wleft-1, strlen(topic)));
	}
	printf("%s", tbar);
	free(cserv);
	free(cchan);
	free(cnick);
	resetcol();
	printf("\n");
	printf(LOCATE, height-1, 1);
}
