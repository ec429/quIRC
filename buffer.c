/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-11 Edward Cree

	See quirc.c for license information
	buffer: multiple-buffer control
*/

#include "buffer.h"

int init_start_buffer(void)
{
	s_buf.nlines=0;
	s_buf.lt=NULL;
	s_buf.lc=NULL;
	s_buf.ts=NULL;
	s_buf.errs=0;
	return(0);
}

int add_to_start_buffer(colour lc, char *lt)
{
	s_buf.nlines++;
	char **nlt;colour *nlc;time_t *nts;
	nlt=(char **)realloc(s_buf.lt, s_buf.nlines*sizeof(char *));
	if(!nlt)
	{
		s_buf.nlines--;
		s_buf.errs++;
		return(1);
	}
	s_buf.lt=nlt;
	s_buf.lt[s_buf.nlines-1]=strdup(lt);
	nlc=(colour *)realloc(s_buf.lc, s_buf.nlines*sizeof(colour));
	if(!nlc)
	{
		s_buf.nlines--;
		s_buf.errs++;
		return(1);
	}
	s_buf.lc=nlc;
	s_buf.lc[s_buf.nlines-1]=lc;
	nts=(time_t *)realloc(s_buf.ts, s_buf.nlines*sizeof(time_t));
	if(!nts)
	{
		s_buf.nlines--;
		s_buf.errs++;
		return(1);
	}
	s_buf.ts=nts;
	s_buf.ts[s_buf.nlines-1]=time(NULL);
	return(0);
}

int asb_failsafe(colour lc, char *lt)
{
	int e=0;
	if((e=add_to_start_buffer(lc, lt)))
	{
		fprintf(stderr, "init[%d]: %s\n", e, lt);
	}
	return(e);
}

int free_start_buffer(void)
{
	int i;
	if(s_buf.lt)
	{
		for(i=0;i<s_buf.nlines;i++)
		{
			if(s_buf.lt[i]) free(s_buf.lt[i]);
		}
		free(s_buf.lt);
		s_buf.lt=NULL;
	}
	if(s_buf.lc) free(s_buf.lc);
	if(s_buf.ts) free(s_buf.ts);
	s_buf.nlines=0;
	return(0);
}

int initialise_buffers(int buflines)
{
	bufs=malloc(sizeof(buffer));
	if(!bufs)
		return(1);
	init_buffer(0, STATUS, "status", buflines); // buf 0 is always STATUS
	nbufs=1;
	cbuf=0;
	bufs[0].live=true; // STATUS is never dead
	bufs[0].nick=nick;
	bufs[0].ilist=igns;
	add_to_buffer(0, c_status, GPL_TAIL, "quirc -- ");
	return(0);
}

int init_buffer(int buf, btype type, char *bname, int nlines)
{
	bufs[buf].type=type;
	bufs[buf].bname=strdup(bname);
	bufs[buf].realsname=NULL;
	bufs[buf].nlist=NULL;
	bufs[buf].ilist=NULL;
	bufs[buf].handle=0;
	bufs[buf].server=0;
	bufs[buf].nick=NULL;
	bufs[buf].topic=NULL;
	bufs[buf].nlines=nlines;
	bufs[buf].ptr=0;
	bufs[buf].scroll=0;
	bufs[buf].ascroll=0;
	bufs[buf].lc=malloc(nlines*sizeof(colour));
	bufs[buf].lt=malloc(nlines*sizeof(char *));
	int i;
	for(i=0;i<bufs[buf].nlines;i++)
	{
		bufs[buf].lt[i]=NULL;
	}
	bufs[buf].ltag=malloc(nlines*sizeof(char *));
	for(i=0;i<bufs[buf].nlines;i++)
	{
		bufs[buf].ltag[i]=NULL;
	}
	bufs[buf].lpl=malloc(nlines*sizeof(int));
	bufs[buf].lpt=malloc(nlines*sizeof(char **));
	for(i=0;i<nlines;i++)
	{
		bufs[buf].lpl[i]=0;
		bufs[buf].lpt[i]=NULL;
	}
	bufs[buf].dirty=false;
	bufs[buf].ts=malloc(nlines*sizeof(time_t));
	bufs[buf].filled=false;
	bufs[buf].alert=false;
	bufs[buf].hi_alert=0;
	bufs[buf].ping=0;
	bufs[buf].last=time(NULL);
	bufs[buf].namreply=false;
	bufs[buf].live=false;
	bufs[buf].conninpr=false;
	initibuf(&bufs[buf].input);
	bufs[buf].casemapping=RFC1459;
	bufs[buf].prefixes=NULL;
	bufs[buf].autoent=NULL;
	return(0);
}

int free_buffer(int buf)
{
	if(bufs[buf].live)
	{
		add_to_buffer(buf, c_err, "Buffer is still live!", "free_buffer:");
		return(1);
	}
	else
	{
		if(bufs[buf].bname) free(bufs[buf].bname);
		if(bufs[buf].realsname) free(bufs[buf].realsname);
		n_free(bufs[buf].nlist);
		bufs[buf].nlist=NULL;
		n_free(bufs[buf].ilist);
		bufs[buf].ilist=NULL;
		if(bufs[buf].lc) free(bufs[buf].lc);
		if(bufs[buf].topic) free(bufs[buf].topic);
		int l;
		if(bufs[buf].lt)
		{
			for(l=0;l<bufs[buf].nlines;l++)
				if(bufs[buf].lt[l]) free(bufs[buf].lt[l]);
			free(bufs[buf].lt);
		}
		if(bufs[buf].ltag)
		{
			for(l=0;l<bufs[buf].nlines;l++)
				if(bufs[buf].ltag[l]) free(bufs[buf].ltag[l]);
			free(bufs[buf].ltag);
		}
		if(bufs[buf].lpt)
		{
			for(l=0;l<bufs[buf].nlines;l++)
			{
				if(bufs[buf].lpt[l])
				{
					if(bufs[buf].lpl)
					{
						int p;
						for(p=0;p<bufs[buf].lpl[l];p++)
						{
							if(bufs[buf].lpt[l][p]) free(bufs[buf].lpt[l][p]);
						}
					}
					free(bufs[buf].lpt[l]);
				}
			}
			free(bufs[buf].lpt);
		}
		if(bufs[buf].lpl) free(bufs[buf].lpl);
		if(bufs[buf].ts) free(bufs[buf].ts);
		freeibuf(&bufs[buf].input);
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

int add_to_buffer(int buf, colour lc, char *lt, char *ltag)
{
	if(buf>=nbufs)
	{
		if(bufs&&buf)
		{
			add_to_buffer(0, c_err, "Line was written to bad buffer!  Contents below.", "add_to_buffer(): ");
			add_to_buffer(0, lc, lt, "");
		}
		return(1);
	}
	int optr=bufs[buf].ptr;
	bool scrollisptr=(bufs[buf].scroll==bufs[buf].ptr)&&(bufs[buf].ascroll==0);
	bufs[buf].lc[bufs[buf].ptr]=lc;
	if(bufs[buf].lt[bufs[buf].ptr]) free(bufs[buf].lt[bufs[buf].ptr]);
	bufs[buf].lt[bufs[buf].ptr]=strdup(lt);
	if(bufs[buf].ltag[bufs[buf].ptr]) free(bufs[buf].ltag[bufs[buf].ptr]);
	bufs[buf].ltag[bufs[buf].ptr]=strdup(ltag);
	bufs[buf].ts[bufs[buf].ptr]=time(NULL);
	bufs[buf].ptr=(bufs[buf].ptr+1)%bufs[buf].nlines;
	if(scrollisptr)
	{
		bufs[buf].scroll=bufs[buf].ptr;
		bufs[buf].ascroll=0;
	}
	if(bufs[buf].ptr==0)
		bufs[buf].filled=true;
	render_line(buf, optr);
	if(buf==cbuf)
	{
		int e=redraw_buffer();
		if(e) return(e);
	}
	else
	{
		bufs[buf].alert=true;
	}
	return(0);
}

int redraw_buffer(void)
{
	if(bufs[cbuf].dirty)
	{
		int e=render_buffer(cbuf);
		if(e) return(e);
		if(bufs[cbuf].dirty) return(1);
	}
	printf(CLS);
	int uline=bufs[cbuf].scroll;
	int pline=bufs[cbuf].ascroll;
	while(pline<0)
	{
		uline--;
		if((bufs[cbuf].filled)&&(uline==bufs[cbuf].ptr))
		{
			uline++;
			pline=0;
			break;
		}
		if(uline<0)
		{
			if(bufs[cbuf].filled)
				uline+=bufs[cbuf].nlines;
			else
			{
				uline=0;
				pline=0;
				break;
			}
		}
		pline+=bufs[cbuf].lpl[uline];
	}
	while(pline>=bufs[cbuf].lpl[uline])
	{
		pline-=bufs[cbuf].lpl[uline];
		if(bufs[cbuf].filled)
		{
			if(uline==bufs[cbuf].ptr)
			{
				pline=0;
				break;
			}
			uline=(uline+1)%bufs[cbuf].nlines;
		}
		else
		{
			if(uline>=bufs[cbuf].ptr)
			{
				uline=bufs[cbuf].ptr;
				pline=0;
				break;
			}
			uline++;
		}
	}
	bufs[cbuf].scroll=uline;
	bufs[cbuf].ascroll=pline;
	int row=height-2;
	bool first=true;
	while(row>(tsb?1:0))
	{
		pline--;
		while(pline<0)
		{
			uline--;
			if(uline<0)
			{
				if(bufs[cbuf].filled)
					uline+=bufs[cbuf].nlines;
				else
				{
					row=-1;
					break;
				}
			}
			if(!first)
			{
				row=-1;
				break;
			}
			if(uline==bufs[cbuf].ptr)
				first=false;
			pline+=bufs[cbuf].lpl[uline];
		}
		if(row<0) break;
		setcolour(bufs[cbuf].lc[uline]);
		if(full_width_colour)
			printf(LOCATE "%s"CLR, row, 0, bufs[cbuf].lpt[uline][pline]);
		else
			printf(LOCATE "%s", row, 0, bufs[cbuf].lpt[uline][pline]);
		row--;
	}
	resetcol();
	while(row>(tsb?1:0))
		printf(LOCATE CLR, row--, 0);
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
		case PRIVATE: // have to scope it for the cstr 'variably modified type'
			{
				char cstr[16+strlen(bufs[cbuf].bname)+strlen(bufs[bufs[cbuf].server].bname)];
				sprintf(cstr, "quIRC - <%s> on %s", bufs[cbuf].bname, bufs[bufs[cbuf].server].bname);
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

int render_buffer(int buf)
{
	if(!bufs[buf].dirty) return(0); // nothing to do...
	int uline,e=0;
	for(uline=0;uline<bufs[buf].nlines;uline++)
	{
		e|=render_line(buf, uline);
		if(e) return(e);
	}
	bufs[buf].dirty=e; // mark it clean
	return(e);
}

int render_line(int buf, int uline)
{
	if(bufs[buf].lpt[uline])
	{
		int pline;
		for(pline=0;pline<bufs[buf].lpl[uline];pline++)
			if(bufs[buf].lpt[uline][pline]) free(bufs[buf].lpt[uline][pline]);
		free(bufs[buf].lpt[uline]);
	}
	char *proc;int l,i;
	init_char(&proc, &l, &i);
	char stamp[32];
	struct tm *td=(utc?gmtime:localtime)(&bufs[buf].ts[uline]);
	switch(ts)
	{
		case 1:
			strftime(stamp, 32, "[%H:%M] ", td);
		break;
		case 2:
			strftime(stamp, 32, "[%H:%M:%S] ", td);
		break;
		case 3:
			if(utc)
				strftime(stamp, 32, "[%H:%M:%S UTC] ", td);
			else
				strftime(stamp, 32, "[%H:%M:%S %z] ", td);
		break;
		case 4:
			snprintf(stamp, 32, "[u+%lu] ", (unsigned long)bufs[buf].ts[uline]);
		break;
		case 0: // no timestamps
		default:
			stamp[0]=0;
		break;
	}
	int x=wordline(stamp, 0, &proc, &l, &i, (colour){.fore=7, .back=0, .hi=false, .ul=false});
	x=wordline(bufs[buf].ltag[uline], x, &proc, &l, &i, bufs[buf].lc[uline]);
	wordline(bufs[buf].lt[uline], x, &proc, &l, &i, bufs[buf].lc[uline]);
	bufs[buf].lpl[uline]=0;
	bufs[buf].lpt[uline]=NULL;
	char *curr=strtok(proc, "\n");
	while(curr)
	{
		int pline=bufs[buf].lpl[uline]++;
		char **nlpt=realloc(bufs[buf].lpt[uline], bufs[buf].lpl[uline]*sizeof(char *));
		if(!nlpt)
		{
			add_to_buffer(0, c_err, "realloc failed; buffer may be corrupted", "render_buffer: ");
			free(proc);
			return(1);
		}
		(bufs[buf].lpt[uline]=nlpt)[pline]=strdup(curr);
		curr=strtok(NULL, "\n");
	}
	free(proc);
	return(0);
}

void in_update(iline inp)
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
			c.back=2; // green
			c.hi=true;
		}
		else if(b==bufs[cbuf].server)
		{
			c.back=4; // blue
			c.ul=true;
		}
		if(bufs[b].hi_alert%2)
		{
			c.fore=6; // cyan
			c.hi=true;
			c.ul=false; // can't have both at once: it's not really a bitmask
		}
		else if(bufs[b].alert)
		{
			c.fore=1; // red
			c.hi=true;
			c.ul=false; // can't have both at once: it's not really a bitmask
		}
		if((!LIVE(b)) && (c.fore!=6))
		{
			c.fore=3; // yellow
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
	if((unsigned)inp.left.i+(unsigned)inp.right.i+1<width)
	{
		char *lh=highlight(inp.left.data?inp.left.data:"");
		char *rh=highlight(inp.right.data?inp.right.data:"");
		printf("%s" SAVEPOS "%s" CLR RESTPOS, lh, rh);
		free(lh);
		free(rh);
	}
	else
	{
		if(inp.left.i<width*0.75)
		{
			char *lh=highlight(inp.left.data?inp.left.data:"");
			char rl[width-inp.left.i-3-max(3, (width-inp.left.i)/4)];
			snprintf(rl, width-inp.left.i-3-max(3, (width-inp.left.i)/4), "%s", inp.right.data);
			char *rlh=highlight(rl);
			char *rrh=highlight(inp.right.data+inp.right.i-max(3, (width-inp.left.i)/4));
			printf("%s" SAVEPOS "%s...%s" CLR RESTPOS, lh, rlh, rrh);
			free(lh);
			free(rlh);
			free(rrh);
		}
		else if(inp.right.i<width*0.75)
		{
			char ll[max(3, (width-inp.right.i)/4)];
			snprintf(ll, max(3, (width-inp.right.i)/4), "%s", inp.left.data);
			char *llh=highlight(ll);
			char *lrh=highlight(inp.left.data+inp.left.i-width+3+inp.right.i+max(3, (width-inp.right.i)/4));
			char *rh=highlight(inp.right.data?inp.right.data:"");
			printf("%s...%s" SAVEPOS "%s" CLR RESTPOS, llh, lrh, rh);
			free(llh);
			free(lrh);
			free(rh);
		}
		else
		{
			int torem=floor((width/4.0)*floor(((inp.left.i-(width/2.0))*4.0/width)+0.5));
			torem=min(torem, inp.left.i-3);
			int c=inp.left.i+4-torem;
			char ll[max(3, c/4)+1];
			snprintf(ll, max(3, c/4)+1, "%s", inp.left.data);
			char *llh=highlight(ll);
			char *lrh=highlight(inp.left.data+torem+max(3, c/4));
			char rl[width-c-2-max(3, (width-c)/4)];
			snprintf(rl, width-c-2-max(3, (width-c)/4), "%s", inp.right.data);
			char *rlh=highlight(rl);
			char *rrh=highlight(inp.right.data+inp.right.i-max(3, (width-c)/4));
			printf("%s...%s" SAVEPOS "%s...%s" CLR RESTPOS, llh, lrh, rlh, rrh);
			free(llh);
			free(lrh);
			free(rlh);
			free(rrh);
		}
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

int e_buf_print(int buf, colour lc, message pkt, char *lead)
{
	int arg;
	int len=(pkt.prefix?strlen(pkt.prefix):0)+strlen(pkt.cmd)+8;
	for(arg=0;arg<pkt.nargs;arg++)
	{
		len+=strlen(pkt.args[arg])+3;
	}
	char text[len];
	if(pkt.prefix)
	{
		strcpy(text, pkt.prefix);
	}
	else
	{
		*text=0;
	}
	strcat(text, " : ");
	strcat(text, pkt.cmd);
	for(arg=0;arg<pkt.nargs;arg++)
	{
		strcat(text, " _ ");
		strcat(text, pkt.args[arg]);
	}
	return(add_to_buffer(buf, lc, text, lead));
}

int transfer_start_buffer(void)
{
	int i,e=0;
	for(i=0;i<s_buf.nlines;i++)
	{
		e|=add_to_buffer(0, s_buf.lc[i], s_buf.lt[i], "init: ");
	}
	return(e);
}

int push_buffer(void)
{
	if(!bufs || transfer_start_buffer())
	{
		if(bufs) add_to_buffer(0, c_err, "Failed to transfer start_buffer", "init[xsb]: ");
		int i;
		for(i=0;i<s_buf.nlines;i++)
		{
			fprintf(stderr, "init[xsb]: %s\n", s_buf.lt[i]);
		}
		if(bufs)
		{
			char msg[32];
			sprintf(msg, "%d messages written to stderr", s_buf.nlines);
			add_to_buffer(0, c_status, msg, "init[xsb]: ");
		}
	}
	if(bufs&&s_buf.errs)
	{
		char msg[80];
		sprintf(msg, "%d messages were written to stderr due to start_buffer errors", s_buf.errs);
		add_to_buffer(0, c_err, msg, "init[errs]: ");
	}
	return(free_start_buffer());
}

void titlebar(void)
{
	printf(LOCATE, 1, 1);
	setcol(0, 7, true, false);
	printf(CLA);
	unsigned int gits;
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
	unsigned int wleft=width-1;
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
	unsigned int i;
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

int findptab(int b, char *src)
{
	int b2;
	for(b2=0;b2<nbufs;b2++)
	{
		if((bufs[b2].server==b)&&(bufs[b2].type==PRIVATE)&&(irc_strcasecmp(bufs[b2].bname, src, bufs[b].casemapping)==0))
			return(b2);
	}
	return(-1);
}
