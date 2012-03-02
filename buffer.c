/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	buffer: multiple-buffer control & buffer rendering
*/

#include "buffer.h"

int init_start_buffer(void)
{
	s_buf.nlines=0;
	s_buf.lt=NULL;
	s_buf.lm=NULL;
	s_buf.ts=NULL;
	s_buf.errs=0;
	return(0);
}

int add_to_start_buffer(mtype lm, const char *lt)
{
	s_buf.nlines++;
	char **nlt;mtype *nlm;time_t *nts;
	nlt=(char **)realloc(s_buf.lt, s_buf.nlines*sizeof(char *));
	if(!nlt)
	{
		s_buf.nlines--;
		s_buf.errs++;
		return(1);
	}
	s_buf.lt=nlt;
	s_buf.lt[s_buf.nlines-1]=strdup(lt);
	nlm=(mtype *)realloc(s_buf.lm, s_buf.nlines*sizeof(mtype));
	if(!nlm)
	{
		s_buf.nlines--;
		s_buf.errs++;
		return(1);
	}
	s_buf.lm=nlm;
	s_buf.lm[s_buf.nlines-1]=lm;
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

int asb_failsafe(mtype lm, const char *lt)
{
	int e=0;
	if((e=add_to_start_buffer(lm, lt)))
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
			free(s_buf.lt[i]);
		}
		free(s_buf.lt);
		s_buf.lt=NULL;
	}
	free(s_buf.lm);
	free(s_buf.ts);
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
	add_to_buffer(0, STA, QUIET, 0, false, GPL_TAIL, "quirc -- ");
	return(0);
}

int init_buffer(int buf, btype type, const char *bname, int nlines)
{
	bufs[buf].type=type;
	bufs[buf].bname=strdup(bname);
	bufs[buf].realsname=NULL;
	bufs[buf].nlist=NULL;
	bufs[buf].us=NULL;
	bufs[buf].ilist=NULL;
	bufs[buf].handle=0;
	bufs[buf].server=0;
	bufs[buf].nick=NULL;
	bufs[buf].topic=NULL;
	bufs[buf].nlines=nlines;
	bufs[buf].ptr=0;
	bufs[buf].scroll=0;
	bufs[buf].ascroll=0;
	bufs[buf].lm=malloc(nlines*sizeof(mtype));
	bufs[buf].lq=malloc(nlines*sizeof(prio));
	bufs[buf].lp=malloc(nlines);
	bufs[buf].ls=malloc(nlines*sizeof(bool));
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
	bufs[buf].lpc=malloc(nlines*sizeof(colour));
	bufs[buf].lpt=malloc(nlines*sizeof(char **));
	for(i=0;i<nlines;i++)
	{
		bufs[buf].lpl[i]=0;
		bufs[buf].lpc[i]=(colour){.fore=7, .back=0, .hi=false, .ul=false};
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
	if(type==SERVER)
	{
		bufs[buf].npfx=2;
		bufs[buf].prefixes=malloc(2*sizeof(prefix));
		bufs[buf].prefixes[0]=(prefix){.letter='o', .pfx='@'};
		bufs[buf].prefixes[1]=(prefix){.letter='v', .pfx='+'};
	}
	else
	{
		bufs[buf].npfx=0;
		bufs[buf].prefixes=NULL;
	}
	bufs[buf].autoent=NULL;
	return(0);
}

int free_buffer(int buf)
{
	if(bufs[buf].live)
	{
		add_to_buffer(buf, ERR, NORMAL, 0, false, "Buffer is still live!", "free_buffer:");
		return(1);
	}
	else
	{
		free(bufs[buf].bname);
		free(bufs[buf].realsname);
		n_free(bufs[buf].nlist);
		bufs[buf].nlist=NULL;
		n_free(bufs[buf].ilist);
		bufs[buf].ilist=NULL;
		free(bufs[buf].lm);
		free(bufs[buf].lq);
		free(bufs[buf].lp);
		free(bufs[buf].ls);
		free(bufs[buf].topic);
		int l;
		if(bufs[buf].lt)
		{
			for(l=0;l<bufs[buf].nlines;l++)
				free(bufs[buf].lt[l]);
			free(bufs[buf].lt);
		}
		if(bufs[buf].ltag)
		{
			for(l=0;l<bufs[buf].nlines;l++)
				free(bufs[buf].ltag[l]);
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
							free(bufs[buf].lpt[l][p]);
						}
					}
					free(bufs[buf].lpt[l]);
				}
			}
			free(bufs[buf].lpt);
		}
		free(bufs[buf].lpl);
		free(bufs[buf].lpc);
		free(bufs[buf].ts);
		freeibuf(&bufs[buf].input);
		free(bufs[buf].prefixes);
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
				bufs[b].handle=0; // just in case
			}
			else if(bufs[b].server>buf)
			{
				bufs[b].server--;
			}
		}
		if(nbufs && (force_redraw<3)) redraw_buffer();
		return(0);
	}
}

int add_to_buffer(int buf, mtype lm, prio lq, char lp, bool ls, const char *lt, const char *ltag)
{
	if(buf>=nbufs)
	{
		if(bufs&&buf)
		{
			add_to_buffer(0, ERR, NORMAL, 0, false, "Line was written to bad buffer!  Contents below.", "add_to_buffer(): ");
			add_to_buffer(0, lm, NORMAL, lp, ls, lt, ltag);
		}
		return(1);
	}
	int optr=bufs[buf].ptr;
	bool scrollisptr=(bufs[buf].scroll==bufs[buf].ptr)&&(bufs[buf].ascroll==0);
	bufs[buf].lm[bufs[buf].ptr]=lm;
	bufs[buf].lq[bufs[buf].ptr]=lq;
	bufs[buf].lp[bufs[buf].ptr]=lp;
	bufs[buf].ls[bufs[buf].ptr]=ls;
	free(bufs[buf].lt[bufs[buf].ptr]);
	bufs[buf].lt[bufs[buf].ptr]=strdup(lt);
	free(bufs[buf].ltag[bufs[buf].ptr]);
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
	if(uline==bufs[cbuf].ptr)
	{
		pline=0;
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
	while(row>(tsb?1:0))
	{
		bool breakit=false;
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
					breakit=true;
					pline=0;
					break;
				}
			}
			if(uline==bufs[cbuf].ptr)
			{
				breakit=true;
				break;
			}
			pline+=bufs[cbuf].lpl[uline];
		}
		if(breakit) break;
		setcolour(bufs[cbuf].lpc[uline]);
		locate(row, 0);
		fputs(bufs[cbuf].lpt[uline][pline], stdout);
		if(!full_width_colour) resetcol();
		clr();
		row--;
	}
	resetcol();
	while(row>(tsb?1:0))
	{
		locate(row--, 0);
		clr();
	}
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

int mark_buffer_dirty(int buf)
{
	bufs[buf].dirty=true;
	return(0);
}

int render_buffer(int buf)
{
	if(!bufs[buf].dirty) return(0); // nothing to do...
	int uline,e=0;
	for(uline=0;uline<(bufs[buf].filled?bufs[buf].nlines:bufs[buf].ptr);uline++)
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
			free(bufs[buf].lpt[uline][pline]);
		free(bufs[buf].lpt[uline]);
	}
	if( // this is quite a complicated conditional, so I've split it up.  It handles conference mode, quiet mode and debug mode
		(bufs[buf].conf&&(
			(bufs[buf].lm[uline]==JOIN)
			||(bufs[buf].lm[uline]==PART)
			||(bufs[buf].lm[uline]==NICK)
			||(bufs[buf].lm[uline]==MODE)
			||(bufs[buf].lm[uline]==QUIT)
			)
		)
		||
			(quiet&&(bufs[buf].lq[uline]==QUIET))
		||
			(!debug&&(bufs[buf].lq[uline]==DEBUG))
		)
	{
		bufs[buf].lpt[uline]=NULL;
		bufs[buf].lpl[uline]=0;
		return(0);
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
	colour c={.fore=7, .back=0, .hi=false, .ul=false};
	int x=wordline(stamp, 0, &proc, &l, &i, c);
	char *tag=strdup(bufs[buf].ltag[uline]?bufs[buf].ltag[uline]:"");
	switch(bufs[buf].lm[uline])
	{
		case MSG:
		{
			c=c_msg[bufs[buf].ls[uline]?1:0];
			char mk[6]="<%s> ";
			if(show_prefix&&bufs[buf].lp[uline])
			{
				mk[0]=mk[3]=bufs[buf].lp[uline];
			}
			crush(&tag, maxnlen);
			char *ntag=mktag(mk, tag);
			free(tag);
			tag=ntag;
		}
		break;
		case NOTICE:
		{
			c=c_notice[bufs[buf].ls[uline]?1:0];
			if(*tag)
			{
				crush(&tag, maxnlen);
				char *ntag=mktag("(from %s) ", tag);
				free(tag);
				tag=ntag;
			}
		}
		break;
		case PREFORMAT:
			c=c_notice[bufs[buf].ls[uline]?1:0];
		break;
		case ACT:
		{
			c=c_actn[bufs[buf].ls[uline]?1:0];
			crush(&tag, maxnlen);
			char *ntag=mktag(" %s  ", tag);
			free(tag);
			tag=ntag;
		}
		break;
		case JOIN:
			c=c_join[bufs[buf].ls[uline]?1:0];
			goto eqtag;
		case PART:
			c=c_part[bufs[buf].ls[uline]?1:0];
			goto eqtag;
		case QUIT:
			c=c_quit[bufs[buf].ls[uline]?1:0];
			goto eqtag;
		case QUIT_PREFORMAT:
			c=c_quit[bufs[buf].ls[uline]?1:0];
		break;
		case NICK:
		{
			c=c_nick[bufs[buf].ls[uline]?1:0];
			eqtag:
			crush(&tag, maxnlen);
			char *ntag=mktag("=%s= ", tag);
			free(tag);
			tag=ntag;
		}
		break;
		case MODE:
			c=c_nick[bufs[buf].ls[uline]?1:0];
		break;
		case STA:
			c=c_status;
		break;
		case ERR:
			c=c_err;
		break;
		case UNK:
			c=c_unk;
		break;
		case UNN:
			c=c_unn;
		break;
		default:
		break;
	}
	x=wordline(tag, x, &proc, &l, &i, c);
	free(tag);
	wordline(bufs[buf].lt[uline], x, &proc, &l, &i, c);
	bufs[buf].lpl[uline]=0;
	bufs[buf].lpt[uline]=NULL;
	bufs[buf].lpc[uline]=c;
	char *curr=strtok(proc, "\n");
	while(curr)
	{
		int pline=bufs[buf].lpl[uline]++;
		char **nlpt=realloc(bufs[buf].lpt[uline], bufs[buf].lpl[uline]*sizeof(char *));
		if(!nlpt)
		{
			add_to_buffer(0, ERR, NORMAL, 0, false, "realloc failed; buffer may be corrupted", "render_buffer: ");
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
	locate(height-1, 1);
	// tab strip
	int mbw = (width-1)/nbufs;
	if(mbw>1)
	{
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
			}
			else if(bufs[b].alert)
			{
				c.fore=1; // red
				c.hi=true;
			}
			if((!LIVE(b)) && (c.fore!=6))
			{
				c.fore=3; // yellow
				c.hi=true;
			}
			setcolour(c);
			putchar(brack[0]);
			if(mbw>3)
			{
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
			}
			if(mbw>2)
				putchar(brack[1]);
			c.fore=7;
			c.back=hilite_tabstrip?5:0;
			c.hi=c.ul=false;
			setcolour(c);
		}
	}
	else
	{
		setcolour((colour){.fore=0, .back=1, .hi=true, .ul=false});
		printf("buf %u [0 to %u]", cbuf, nbufs-1);
	}
	clr();
	resetcol();
	putchar('\n');
	unsigned int wwidth=width;
	char stamp[32];
	stamp[0]=0;
	if(its)
	{
		time_t t=time(NULL);
		struct tm *td=(utc?gmtime:localtime)(&t);
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
				snprintf(stamp, 32, "[u+%lu] ", (unsigned long)t);
			break;
			case 0: // no timestamps
			default:
			break;
		}
		if(strlen(stamp)+25>wwidth)
		{
			stamp[0]=0;
			its=false;
			add_to_buffer(0, STA, NORMAL, 0, false, "disabled due to insufficient display width", "its: ");
		}
		wwidth-=strlen(stamp);
	}
	// input
	if((unsigned)inp.left.i+(unsigned)inp.right.i+1<wwidth)
	{
		char *lh=highlight(inp.left.data?inp.left.data:"");
		char *rh=highlight(inp.right.data?inp.right.data:"");
		fputs(stamp, stdout);
		fputs(lh, stdout);
		savepos();
		fputs(rh, stdout);
		clr();
		restpos();
		free(lh);
		free(rh);
	}
	else
	{
		if(inp.left.i<wwidth*0.75)
		{
			char *lh=highlight(inp.left.data?inp.left.data:"");
			char rl[wwidth-inp.left.i-3-max(3, (wwidth-inp.left.i)/4)];
			snprintf(rl, wwidth-inp.left.i-3-max(3, (wwidth-inp.left.i)/4), "%s", inp.right.data);
			char *rlh=highlight(rl);
			char *rrh=highlight(inp.right.data+inp.right.i-max(3, (wwidth-inp.left.i)/4));
			fputs(stamp, stdout);
			fputs(lh, stdout);
			savepos();
			printf("%s...%s", rlh, rrh);
			clr();
			restpos();
			free(lh);
			free(rlh);
			free(rrh);
		}
		else if(inp.right.i<wwidth*0.75)
		{
			char ll[max(3, (wwidth-inp.right.i)/4)];
			snprintf(ll, max(3, (wwidth-inp.right.i)/4), "%s", inp.left.data);
			char *llh=highlight(ll);
			char *lrh=highlight(inp.left.data+inp.left.i-wwidth+3+inp.right.i+max(3, (wwidth-inp.right.i)/4));
			char *rh=highlight(inp.right.data?inp.right.data:"");
			fputs(stamp, stdout);
			printf("%s...%s", llh, lrh);
			savepos();
			fputs(rh, stdout);
			restpos();
			free(llh);
			free(lrh);
			free(rh);
		}
		else
		{
			int torem=floor((wwidth/4.0)*floor(((inp.left.i-(wwidth/2.0))*4.0/wwidth)+0.5));
			torem=min(torem, inp.left.i-3);
			int c=inp.left.i+4-torem;
			char ll[max(3, c/4)+1];
			snprintf(ll, max(3, c/4)+1, "%s", inp.left.data);
			char *llh=highlight(ll);
			char *lrh=highlight(inp.left.data+torem+max(3, c/4));
			char rl[wwidth-c-2-max(3, (wwidth-c)/4)];
			snprintf(rl, wwidth-c-2-max(3, (wwidth-c)/4), "%s", inp.right.data);
			char *rlh=highlight(rl);
			char *rrh=highlight(inp.right.data+inp.right.i-max(3, (wwidth-c)/4));
			fputs(stamp, stdout);
			printf("%s...%s", llh, lrh);
			savepos();
			printf("%s...%s", rlh, rrh);
			clr();
			restpos();
			free(llh);
			free(lrh);
			free(rlh);
			free(rrh);
		}
	}
	fflush(stdout);
}

char *highlight(const char *src)
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
		else if(!isprint(*src))
		{
			s_setcol(2, 0, 1, 1, &rv, &l, &i);
			append_char(&rv, &l, &i, '\\');
			char obuf[16];
			snprintf(obuf, 16, "%03o", (unsigned char)*src);
			append_str(&rv, &l, &i, obuf);
			s_setcol(7, 0, 0, 0, &rv, &l, &i);
		}
		else
		{
			append_char(&rv, &l, &i, *src);
		}
		src++;
	}
	return(rv);
}

int e_buf_print(int buf, mtype lm, message pkt, const char *lead)
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
	return(add_to_buffer(buf, lm, QUIET, 0, false, text, lead));
}

int transfer_start_buffer(void)
{
	int i,e=0;
	for(i=0;i<s_buf.nlines;i++)
	{
		e|=add_to_buffer(0, s_buf.lm[i], QUIET, 0, false, s_buf.lt[i], "init: ");
	}
	return(e);
}

int push_buffer(void)
{
	if(!bufs || transfer_start_buffer())
	{
		if(bufs) add_to_buffer(0, ERR, NORMAL, 0, false, "Failed to transfer start_buffer", "init[xsb]: ");
		int i;
		for(i=0;i<s_buf.nlines;i++)
		{
			fprintf(stderr, "init[xsb]: %s\n", s_buf.lt[i]);
		}
		if(bufs)
		{
			char msg[32];
			sprintf(msg, "%d messages written to stderr", s_buf.nlines);
			add_to_buffer(0, STA, NORMAL, 0, false, msg, "init[xsb]: ");
		}
	}
	if(bufs&&s_buf.errs)
	{
		char msg[80];
		sprintf(msg, "%d messages were written to stderr due to start_buffer errors", s_buf.errs);
		add_to_buffer(0, ERR, NORMAL, 0, false, msg, "init[errs]: ");
	}
	return(free_start_buffer());
}

void titlebar(void)
{
	locate(1, 1);
	setcol(0, 7, true, false);
	clr();
	unsigned int gits;
	sscanf(VERSION_TXT, "%u", &gits);
	const char *hashgit=strchr(VERSION_TXT, ' ');
	if(hashgit)
		hashgit++;
	char *cserv=strdup(bufs[bufs[cbuf].server].bname?bufs[bufs[cbuf].server].bname:"");
	char *cnick=strdup(bufs[bufs[cbuf].server].nick?bufs[bufs[cbuf].server].nick:"");
	char *cchan=strdup(((bufs[cbuf].type==CHANNEL)||(bufs[cbuf].type==PRIVATE))&&bufs[cbuf].bname?bufs[cbuf].bname:"");
	size_t chanlen=strlen(cchan)+1, nicklen=strlen(cnick)+1;
	if(bufs[cbuf].type==CHANNEL)
	{
		if(bufs[cbuf].npfx) chanlen+=2+bufs[cbuf].npfx;
		if((bufs[cbuf].us)&&(bufs[cbuf].us->npfx)) nicklen+=2+bufs[cbuf].us->npfx;
	}
	char *topic=(bufs[cbuf].type==CHANNEL)?bufs[cbuf].topic:NULL;
	scrush(&cserv, 16);
	crush(&cnick, 16);
	crush(&cchan, 16);
	char tbar[width];
	unsigned int wleft=width-1;
	bool use[8]; // #chan(modes)	nick(modes)	quIRC	version	gits	ghashgit	server	topic...
	char version[32];
	sprintf(version, "%hhu.%hhu.%hhu", VERSION_MAJ, VERSION_MIN, VERSION_REV);
	char vgits[8];
	sprintf(vgits, "%hhu", gits);
	memset(use, 0, sizeof(bool[8]));
	if(*cchan && (wleft>=chanlen))
	{
		use[0]=true;
		wleft-=chanlen;
	}
	if(*cnick && (wleft>=nicklen))
	{
		use[1]=true;
		wleft-=nicklen;
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
	// 2quIRC 3version 4gits 5ghashgit 6server 0chan(m) 1nick(m) 7topic
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
		p+=strlen(cchan);
		if((bufs[cbuf].type==CHANNEL)&&(bufs[cbuf].npfx))
		{
			tbar[p++]='(';
			for(unsigned int i=0;i<bufs[cbuf].npfx;i++)
				tbar[p++]=bufs[cbuf].prefixes[i].letter;
			tbar[p++]=')';
		}
		p++;
	}
	if(use[1])
	{
		memcpy(tbar+p, cnick, strlen(cnick));
		p+=strlen(cnick);
		if((bufs[cbuf].type==CHANNEL)&&(bufs[cbuf].us)&&(bufs[cbuf].us->npfx))
		{
			tbar[p++]='(';
			for(unsigned int i=0;i<bufs[cbuf].us->npfx;i++)
				tbar[p++]=bufs[cbuf].us->prefixes[i].letter;
			tbar[p++]=')';
		}
		p++;
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
	locate(height-1, 1);
}

int findptab(int b, const char *src)
{
	int b2;
	for(b2=0;b2<nbufs;b2++)
	{
		if((bufs[b2].server==b)&&(bufs[b2].type==PRIVATE)&&(irc_strcasecmp(bufs[b2].bname, src, bufs[b].casemapping)==0))
			return(b2);
	}
	return(-1);
}
