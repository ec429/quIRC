/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	input: handle input routines
*/

#include "input.h"
#include "logging.h"

int inputchar(iline *inp, int *state)
{
	int c=getchar();
	if((c==0)||(c==EOF)) // stdin is set to non-blocking, so this may happen
		return(0);
	char *seq;size_t sl,si;
	int mod=-1;
	init_char(&seq, &sl, &si);
	bool match=true;
	while(match&&(mod<0))
	{
		append_char(&seq, &sl, &si, c);
		match=false;
		for(int i=0;i<nkeys;i++)
		{
			if(strncmp(seq, kmap[i].mod, si)==0)
			{	
				if(kmap[i].mod[si]==0)
				{
					mod=i;
					break;
				}
				else
				{
					match=true;
				}
			}
		}
		if(match&&(mod<0))
		{
			c=getchar();
			if((c==0)||(c==EOF))
			{
				match=false;
			}
		}
	}
	if(mod<0)
	{
		while(si>1) ungetc(seq[--si], stdin);
		append_char(&inp->left.data, &inp->left.l, &inp->left.i, seq[0]);
	}
	free(seq);
	if(c!='\t')
		ttab=false;
	if(mod==KEY_BS) // backspace
		back_ichar(&inp->left);
	else if((mod<0) && (c<32)) // this also stomps on the newline
	{
		back_ichar(&inp->left);
		if(c==8) // C-h ~= backspace
		{
			back_ichar(&inp->left);
		}
		if(c==1) // C-a ~= home
		{
			i_home(inp);
		}
		if(c==5) // C-e ~= end
		{
			i_end(inp);
		}
		if(c==3) // C-c ~= clear
		{
			ifree(inp);
		}
		if(c==24) // C-x ~= clear to left
		{
			free(inp->left.data);
			inp->left.data=NULL;
			inp->left.i=inp->left.l=0;
		}
		if(c==11) // C-k ~= clear to right
		{
			free(inp->right.data);
			inp->right.data=NULL;
			inp->right.i=inp->right.l=0;
		}
		if(c==23) // C-w ~= backspace word
		{
			while(back_ichar(&inp->left)==' ');
			while(!strchr(" ", back_ichar(&inp->left)));
			if(inp->left.i)
				append_char(&inp->left.data, &inp->left.l, &inp->left.i, ' ');
		}
		if(c=='\t') // tab completion of nicks
		{
			size_t sp=inp->left.i;
			if(sp) sp--;
			while(sp>0 && !strchr(" \t", inp->left.data[sp-1]))
				sp--;
			name *curr=bufs[cbuf].nlist;
			name *found=NULL;
			size_t count=0, mlen=0;
			while(curr)
			{
				if((inp->left.i==sp) || (irc_strncasecmp(inp->left.data+sp, curr->data, inp->left.i-sp, bufs[cbuf].casemapping)==0))
				{
					name *old=found;
					n_add(&found, curr->data, bufs[cbuf].casemapping);
					if(old&&(old->data))
					{
						size_t i;
						for(i=0;i<mlen;i++)
						{
							if(irc_to_upper(curr->data[i], bufs[cbuf].casemapping)!=irc_to_upper(old->data[i], bufs[cbuf].casemapping))
								break;
						}
						mlen=i;
					}
					else
					{
						mlen=strlen(curr->data);
					}
					count++;
				}
				if(curr)
					curr=curr->next;
			}
			if(found)
			{
				if((mlen>inp->left.i-sp)&&(count>1))
				{
					while(sp<inp->left.i)
						back_ichar(&inp->left);
					const char *p=found->data;
					for(size_t i=0;i<mlen;i++)
					{
						if(!p[i]) break;
						if(p[i]=='\\')
							append_char(&inp->left.data, &inp->left.l, &inp->left.i, p[i]);
						append_char(&inp->left.data, &inp->left.l, &inp->left.i, p[i]);
					}
					ttab=false;
				}
				else if((count>16)&&!ttab)
				{
					add_to_buffer(cbuf, STA, NORMAL, 0, false, "Multiple matches (over 16; tab again to list)", "[tab] ");
					ttab=true;
				}
				else if(found->next||(count>1))
				{
					char *fmsg;
					size_t l,i;
					init_char(&fmsg, &l, &i);
					while(found)
					{
						append_str(&fmsg, &l, &i, found->data);
						found=found->next;
						count--;
						if(count)
						{
							append_str(&fmsg, &l, &i, ", ");
						}
					}
					if(!ttab)
						add_to_buffer(cbuf, STA, NORMAL, 0, false, "Multiple matches", "[tab] ");
					add_to_buffer(cbuf, STA, NORMAL, 0, false, fmsg, "[tab] ");
					free(fmsg);
					ttab=false;
				}
				else
				{
					while(sp<inp->left.i)
						back_ichar(&inp->left);
					const char *p=found->data;
					while(*p)
					{
						if(*p=='\\')
							append_char(&inp->left.data, &inp->left.l, &inp->left.i, *p);
						append_char(&inp->left.data, &inp->left.l, &inp->left.i, *p++);
					}
					append_str(&inp->left.data, &inp->left.l, &inp->left.i, ": ");
					ttab=false;
				}
			}
			else
			{
				add_to_buffer(cbuf, STA, NORMAL, 0, false, "No nicks match", "[tab] ");
			}
			n_free(found);
		}
	}
	else if(mod>=0)
	{
		bool gone=false;
		for(int n=1;n<=12;n++)
		{
			if(mod==KEY_F(n))
			{
				cbuf=min(n%12, nbufs-1);
				if(force_redraw<3) redraw_buffer();
			}
		}
		if(mod==KEY_UP) // Up
		{
			int old=bufs[cbuf].input.scroll;
			bufs[cbuf].input.scroll=min(bufs[cbuf].input.scroll+1, bufs[cbuf].input.filled?bufs[cbuf].input.nlines-1:bufs[cbuf].input.ptr);
			if(old!=bufs[cbuf].input.scroll) gone=true;
			if(gone&&!old)
			{
				if(inp->left.i||inp->right.i)
				{
					char out[inp->left.i+inp->right.i+1];
					sprintf(out, "%s%s", inp->left.data?inp->left.data:"", inp->right.data?inp->right.data:"");
					addtoibuf(&bufs[cbuf].input, out);
					bufs[cbuf].input.scroll=2;
				}
			}
		}
		else if(mod==KEY_DOWN) // Down
		{
			gone=true;
			if(!bufs[cbuf].input.scroll)
			{
				if(inp->left.i||inp->right.i)
				{
					char out[inp->left.i+inp->right.i+1];
					sprintf(out, "%s%s", inp->left.data?inp->left.data:"", inp->right.data?inp->right.data:"");
					addtoibuf(&bufs[cbuf].input, out);
					bufs[cbuf].input.scroll=0;
				}
			}
			bufs[cbuf].input.scroll=max(bufs[cbuf].input.scroll-1, 0);
		}
		if(gone&&(bufs[cbuf].input.ptr||bufs[cbuf].input.filled))
		{
			if(bufs[cbuf].input.scroll)
			{
				char *ln=bufs[cbuf].input.line[(bufs[cbuf].input.ptr+bufs[cbuf].input.nlines-bufs[cbuf].input.scroll)%bufs[cbuf].input.nlines];
				if(ln)
				{
					ifree(inp);
					inp->left.data=strdup(ln);inp->left.i=strlen(inp->left.data);inp->left.l=0;
				}
			}
			else
			{
				ifree(inp);
			}
		}
		else if(mod==KEY_RIGHT)
		{
			if(inp->right.data && *inp->right.data)
			{
				append_char(&inp->left.data, &inp->left.l, &inp->left.i, inp->right.data[0]);
				char *nr=strdup(inp->right.data+1);
				free(inp->right.data);
				inp->right.data=nr;
				inp->right.i--;
				inp->right.l=0;
			}
		}
		else if(mod==KEY_LEFT)
		{
			if(inp->left.i)
			{
				unsigned char e=back_ichar(&inp->left);
				if(e)
				{
					char *nr=(char *)malloc(inp->right.i+2);
					*nr=e;
					if(inp->right.data)
					{
						strcpy(nr+1, inp->right.data);
						free(inp->right.data);
					}
					else
					{
						nr[1]=0;
					}
					inp->right.data=nr;
					inp->right.i++;
					inp->right.l=inp->right.i+1;
				}
			}
		}
		else if(mod==KEY_HOME)
			i_home(inp);
		else if(mod==KEY_END)
			i_end(inp);
		else if(mod==KEY_DELETE)
		{
			if(inp->right.data && inp->right.i)
			{
				char *nr=strdup(inp->right.data+1);
				free(inp->right.data);
				inp->right.data=nr;
				inp->right.l=inp->right.i--;
			}
		}
		else if(mod==KEY_CPGUP)
		{
			bufs[cbuf].ascroll-=height-(tsb?3:2);
			if(force_redraw<3) redraw_buffer();
		}
		else if(mod==KEY_CPGDN)
		{
			bufs[cbuf].ascroll+=height-(tsb?3:2);
			if(force_redraw<3) redraw_buffer();
		}
		else
		{
			bool gone=false;
			if(mod==KEY_PGUP)
			{
				int old=bufs[cbuf].input.scroll;
				bufs[cbuf].input.scroll=min(bufs[cbuf].input.scroll+10, bufs[cbuf].input.filled?bufs[cbuf].input.nlines-1:bufs[cbuf].input.ptr);
				if(old!=bufs[cbuf].input.scroll) gone=true;
				if(gone&&!old)
				{
					if(inp->left.i||inp->right.i)
					{
						char out[inp->left.i+inp->right.i+1];
						sprintf(out, "%s%s", inp->left.data?inp->left.data:"", inp->right.data?inp->right.data:"");
						addtoibuf(&bufs[cbuf].input, out);
						bufs[cbuf].input.scroll=2;
					}
				}
			}
			else if(mod==KEY_PGDN)
			{
				gone=true;
				if(!bufs[cbuf].input.scroll)
				{
					if(inp->left.i||inp->right.i)
					{
						char out[inp->left.i+inp->right.i+1];
						sprintf(out, "%s%s", inp->left.data?inp->left.data:"", inp->right.data?inp->right.data:"");
						addtoibuf(&bufs[cbuf].input, out);
						bufs[cbuf].input.scroll=0;
					}
				}
				bufs[cbuf].input.scroll=max(bufs[cbuf].input.scroll-10, 0);
			}
			if(gone&&(bufs[cbuf].input.ptr||bufs[cbuf].input.filled))
			{
				if(bufs[cbuf].input.scroll)
				{
					char *ln=bufs[cbuf].input.line[(bufs[cbuf].input.ptr+bufs[cbuf].input.nlines-bufs[cbuf].input.scroll)%bufs[cbuf].input.nlines];
					if(ln)
					{
						ifree(inp);
						inp->left.data=strdup(ln);inp->left.i=strlen(inp->left.data);inp->left.l=0;
					}
				}
				else
				{
					ifree(inp);
				}
			}
			else if((mod==KEY_SLEFT)||(mod==KEY_CLEFT)||(mod==KEY_ALEFT))
			{
				cbuf=max(cbuf-1, 0);
				if(force_redraw<3) redraw_buffer();
			}
			else if((mod==KEY_SRIGHT)||(mod==KEY_CRIGHT)||(mod==KEY_ARIGHT))
			{
				cbuf=min(cbuf+1, nbufs-1);
				if(force_redraw<3) redraw_buffer();
			}
			else if((mod==KEY_CUP)||(mod==KEY_AUP))
			{
				bufs[cbuf].ascroll--;
				if(force_redraw<3) redraw_buffer();
			}
			else if((mod==KEY_CDOWN)||(mod==KEY_ADOWN))
			{
				bufs[cbuf].ascroll++;
				if(force_redraw<3) redraw_buffer();
			}
			else if((mod==KEY_SHOME)||(mod==KEY_CHOME)||(mod==KEY_AHOME))
			{
				bufs[cbuf].scroll=bufs[cbuf].filled?(bufs[cbuf].ptr+1)%bufs[cbuf].nlines:0;
				bufs[cbuf].ascroll=0;
				if(force_redraw<3) redraw_buffer();
			}
			else if((mod==KEY_SEND)||(mod==KEY_CEND)||(mod==KEY_AEND))
			{
				bufs[cbuf].scroll=bufs[cbuf].ptr;
				bufs[cbuf].ascroll=0;
				if(force_redraw<3) redraw_buffer();
			}
		}
	}
	else if((c&0xe0)==0xc0) // 110xxxxx -> 2 bytes of UTF-8
	{
		int d=getchar();
		if(d&&(d!=EOF)&&((d&0xc0)==0x80)) // 10xxxxxx - UTF middlebyte
			append_char(&inp->left.data, &inp->left.l, &inp->left.i, d);
		else
			ungetc(d, stdin);
	}
	else if((c&0xf0)==0xe0) // 1110xxxx -> 3 bytes of UTF-8
	{
		int d=getchar();
		if(d&&(d!=EOF)&&((d&0xc0)==0x80)) // 10xxxxxx - UTF middlebyte
		{
			int e=getchar();
			if(e&&(e!=EOF)&&((e&0xc0)==0x80)) // 10xxxxxx - UTF middlebyte
			{
				append_char(&inp->left.data, &inp->left.l, &inp->left.i, d);
				append_char(&inp->left.data, &inp->left.l, &inp->left.i, e);
			}
			else
			{
				ungetc(e, stdin);
				ungetc(d, stdin);
			}
		}
		else
			ungetc(d, stdin);
	}
	else if((c&0xf8)==0xf0) // 11110xxx -> 4 bytes of UTF-8
	{
		int d=getchar();
		if(d&&(d!=EOF)&&((d&0xc0)==0x80)) // 10xxxxxx - UTF middlebyte
		{
			int e=getchar();
			if(e&&(e!=EOF)&&((e&0xc0)==0x80)) // 10xxxxxx - UTF middlebyte
			{
				int f=getchar();
				if(f&&(f!=EOF)&&((f&0xc0)==0x80)) // 10xxxxxx - UTF middlebyte
				{
					append_char(&inp->left.data, &inp->left.l, &inp->left.i, d);
					append_char(&inp->left.data, &inp->left.l, &inp->left.i, e);
					append_char(&inp->left.data, &inp->left.l, &inp->left.i, f);
				}
				else
				{
					ungetc(f, stdin);
					ungetc(e, stdin);
					ungetc(d, stdin);
				}
			}
			else
			{
				ungetc(e, stdin);
				ungetc(d, stdin);
			}
		}
		else
			ungetc(d, stdin);
	}
	if(c=='\n')
	{
		*state=3;
		char out[inp->left.i+inp->right.i+1];
		sprintf(out, "%s%s", inp->left.data?inp->left.data:"", inp->right.data?inp->right.data:"");
		addtoibuf(&bufs[cbuf].input, out);
		ifree(inp);
	}
	else
	{
		in_update(*inp);
	}
	return(0);
}

char * slash_dequote(char *inp)
{
	size_t l=strlen(inp)+1;
	char *rv=(char *)malloc(l+1); // we only get shorter, so this will be enough
	unsigned int o=0;
	while((*inp) && (o<l)) // o>=l should never happen, but it's covered just in case
	{
		if(*inp=='\\') // \n, \r, \\, \ooo (\0 remains escaped)
		{
			char c=*++inp;
			switch(c)
			{
				case 'n':
					rv[o++]='\n';inp++;
				break;
				case 'r':
					rv[o++]='\r';inp++;
				break;
				case '\\':
					rv[o++]='\\';inp++;
				break;
				case '0': // \000 to \377 are octal escapes
				case '1':
				case '2':
				case '3':
				{
					int digits=0;
					int oval=c-'0'; // Octal VALue
					while(isdigit(inp[1]) && (inp[1]<'8') && (++digits<3))
					{
						oval*=8;
						oval+=(*++inp)-'0';
					}
					if(oval)
					{
						rv[o++]=oval;
					}
					else // \0 is a special case (it remains escaped)
					{
						rv[o++]='\\';
						if(o<l)
							rv[o++]='0';
					}
					inp++;
				}
				break;
				default:
					rv[o++]='\\';
				break;
			}
		}
		else
		{
			rv[o++]=*inp++;
		}
	}
	rv[o]=0;
	return(rv);
}

int cmd_handle(char *inp, char **qmsg, fd_set *master, int *fdmax) // old state=3; return new state
{
	char *cmd=inp+1;
	if(*cmd=='/') //msg sends /msg
		return(talk(cmd));
	char *args=strchr(cmd, ' ');
	if(args) *args++=0;
	if(strcmp(cmd, "close")==0)
	{
		switch(bufs[cbuf].type)
		{
			case STATUS:
				cmd="quit";
			break;
			case SERVER:
				if(bufs[cbuf].live)
				{
					cmd="disconnect";
				}
				else
				{
					free_buffer(cbuf);
					return(0);
				}
			break;
			case CHANNEL:
				if(bufs[cbuf].live)
				{
					cmd="part";
				}
				else
				{
					free_buffer(cbuf);
					return(0);
				}
			break;
			default:
				bufs[cbuf].live=false;
				free_buffer(cbuf);
				return(0);
			break;
		}
	}
	if((strcmp(cmd, "quit")==0)||(strcmp(cmd, "exit")==0))
	{
		if(args) {free(*qmsg); *qmsg=strdup(args);}
		add_to_buffer(cbuf, STA, NORMAL, 0, false, "Exited quirc", "/quit: ");
		return(-1);
	}
	if(strcmp(cmd, "log")==0) // start/stop logging
	{
		if(!args)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a log type or /log -", "/log: ");
			return(0);
		}
		if(bufs[cbuf].logf)
		{
			fclose(bufs[cbuf].logf);
			bufs[cbuf].logf=NULL;
		}
		if(strcmp(args, "-")==0)
		{
			add_to_buffer(cbuf, STA, QUIET, 0, false, "Disabled logging of this buffer", "/log: ");
			return(0);
		}
		else
		{
			char *type=strtok(args, " ");
			if(type)
			{
				char *fn=strtok(NULL, "");
				if(fn)
				{
					logtype logt;
					if(strcasecmp(type, "plain")==0)
						logt=LOGT_PLAIN;
					else if(strcasecmp(type, "symbolic")==0)
						logt=LOGT_SYMBOLIC;
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Unrecognised log type (valid types are: plain, symbolic)", "/log: ");
						return(0);
					}
					FILE *fp=fopen(fn, "a");
					if(!fp)
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Failed to open log file for append", "/log: ");
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, strerror(errno), "fopen: ");
						return(0);
					}
					log_init(fp, logt);
					bufs[cbuf].logf=fp;
					bufs[cbuf].logt=logt;
					add_to_buffer(cbuf, STA, QUIET, 0, false, "Enabled logging of this buffer", "/log: ");
					return(0);
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a log file", "/log: ");
					return(0);
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a log type or /log -", "/log: ");
				return(0);
			}
		}
	}
	if(strcmp(cmd, "set")==0) // set options
	{
		bool osp=show_prefix, odbg=debug, oind=indent;
		unsigned int omln=maxnlen;
		if(args)
		{
			char *opt=strtok(args, " ");
			if(opt)
			{
				char *val=strtok(NULL, "");
#include "config_set.c"
				else if(strcmp(opt, "conf")==0)
				{
					if(bufs[cbuf].type==CHANNEL)
					{
						if(val)
						{
							if(isdigit(*val))
							{
								unsigned int value;
								sscanf(val, "%u", &value);
								bufs[cbuf].conf=value;
							}
							else if(strcmp(val, "+")==0)
							{
								bufs[cbuf].conf=true;
							}
							else if(strcmp(val, "-")==0)
							{
								bufs[cbuf].conf=false;
							}
							else
							{
								add_to_buffer(cbuf, ERR, NORMAL, 0, false, "option 'conf' is boolean, use only 0/1 or -/+ to set", "/set: ");
							}
						}
						else
							bufs[cbuf].conf=true;
						if(bufs[cbuf].conf)
							add_to_buffer(cbuf, STA, QUIET, 0, false, "conference mode enabled for this channel", "/set: ");
						else
							add_to_buffer(cbuf, STA, QUIET, 0, false, "conference mode disabled for this channel", "/set: ");
						mark_buffer_dirty(cbuf);
						redraw_buffer();
					}
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Not a channel!", "/set conf: ");
					}
				}
				else if(strcmp(opt, "uname")==0)
				{
					if(val)
					{
						free(username);
						username=strdup(val);
						add_to_buffer(cbuf, STA, QUIET, 0, false, username, "/set uname ");
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Non-null value required for uname", "/set uname: ");
				}
				else if(strcmp(opt, "fname")==0)
				{
					if(val)
					{
						free(fname);
						fname=strdup(val);
						add_to_buffer(cbuf, STA, QUIET, 0, false, fname, "/set fname ");
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Non-null value required for fname", "/set fname: ");
				}
				else if(strcmp(opt, "pass")==0)
				{
					if(val)
					{
						free(pass);
						pass=strdup(val);
						char *p=val;
						while(*p) *p++='*';
						add_to_buffer(cbuf, STA, QUIET, 0, false, val, "/set pass ");
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Non-null value required for pass", "/set pass: ");
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "No such option!", "/set: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "But what do you want to set?", "/set: ");
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "But what do you want to set?", "/set: ");
		}
		if((show_prefix!=osp)||(maxnlen!=omln)||(indent!=oind))
		{
			for(int b=0;b<nbufs;b++)
				mark_buffer_dirty(b);
		}
		if(debug&&!odbg)
		{
			push_ring(&d_buf, DEBUG);
		}
		else if(odbg&&!debug)
		{
			init_ring(&d_buf);
			d_buf.loop=true;
		}
		return(0);
	}
	if((strcmp(cmd, "server")==0)||(strcmp(cmd, "connect")==0))
	{
		if(args)
		{
			char *server=args;
			char *newport=strchr(server, ':');
			if(newport)
			{
				*newport=0;
				newport++;
			}
			else
			{
				newport=portno;
			}
			int b;
			for(b=1;b<nbufs;b++)
			{
				if((bufs[b].type==SERVER) && (irc_strcasecmp(server, bufs[b].bname, bufs[b].casemapping)==0))
				{
					if(bufs[b].live)
					{
						cbuf=b;
						if(force_redraw<3) redraw_buffer();
						return(0);
					}
					else
					{
						cbuf=b;
						cmd="reconnect";
						if(force_redraw<3) redraw_buffer();
						break;
					}
				}
			}
			if(b>=nbufs)
			{
				char dstr[30+strlen(server)+strlen(newport)];
				sprintf(dstr, "Connecting to %s on port %s...", server, newport);
				#if ASYNCH_NL
				__attribute__((unused)) int *p= fdmax;
				nl_list *nl=irc_connect(server, newport);
				if(nl)
				{
					nl->reconn_b=0;
					add_to_buffer(0, STA, QUIET, 0, false, dstr, "/server: ");
					if(force_redraw<3) redraw_buffer();
				}
				#else
				int serverhandle=irc_connect(server, newport, master, fdmax);
				if(serverhandle)
				{
					bufs=(buffer *)realloc(bufs, ++nbufs*sizeof(buffer));
					init_buffer(nbufs-1, SERVER, server, buflines);
					cbuf=nbufs-1;
					bufs[cbuf].handle=serverhandle;
					bufs[cbuf].nick=strdup(nick);
					bufs[cbuf].server=cbuf;
					bufs[cbuf].conninpr=true;
					add_to_buffer(cbuf, STA, QUIET, 0, false, dstr, "/server: ");
					if(force_redraw<3) redraw_buffer();
				}
				#endif
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a server!", "/server: ");
		}
		return(0);
	}
	if(strcmp(cmd, "reconnect")==0)
	{
		if(bufs[cbuf].server)
		{
			if(!bufs[bufs[cbuf].server].live)
			{
				char *newport;
				if(args)
				{
					newport=args;
				}
				else if(bufs[bufs[cbuf].server].autoent && bufs[bufs[cbuf].server].autoent->portno)
				{
					newport=bufs[bufs[cbuf].server].autoent->portno;
				}
				else
				{
					newport=portno;
				}
				char dstr[30+strlen(bufs[bufs[cbuf].server].serverloc)+strlen(newport)];
				sprintf(dstr, "Connecting to %s on port %s...", bufs[bufs[cbuf].server].serverloc, newport);
				#if ASYNCH_NL
				nl_list *nl=irc_connect(bufs[bufs[cbuf].server].serverloc, newport);
				if(nl)
				{
					nl->reconn_b=bufs[cbuf].server;
					add_to_buffer(bufs[cbuf].server, STA, QUIET, 0, false, dstr, "/server: ");
					if(force_redraw<3) redraw_buffer();
				}
				else
				{
					add_to_buffer(bufs[cbuf].server, ERR, NORMAL, 0, false, "malloc failure (see status)", "/server: ");
					if(force_redraw<3) redraw_buffer();
				}
				#else /* ASYNCH_NL */
				int serverhandle=irc_connect(bufs[bufs[cbuf].server].serverloc, newport, master, fdmax);
				if(serverhandle)
				{
					int b=bufs[cbuf].server;
					bufs[b].handle=serverhandle;
					int b2;
					for(b2=1;b2<nbufs;b2++)
					{
						if(bufs[b2].server==b)
							bufs[b2].handle=serverhandle;
					}
					bufs[cbuf].conninpr=true;
					free(bufs[cbuf].realsname);
					bufs[cbuf].realsname=NULL;
					add_to_buffer(cbuf, STA, QUIET, 0, false, dstr, "/server: ");
				}
				#endif /* ASYNCH_NL */
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Already connected to server", "/reconnect: ");
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/reconnect: ");
		}
		return(0);
	}
	if(strcmp(cmd, "disconnect")==0)
	{
		int b=bufs[cbuf].server;
		if(b>0)
		{
			if(bufs[b].handle)
			{
				if(bufs[b].live)
				{
					char quit[7+strlen(args?args:(qmsg&&*qmsg)?*qmsg:"quIRC quit")];
					sprintf(quit, "QUIT %s", args?args:(qmsg&&*qmsg)?*qmsg:"quIRC quit");
					irc_tx(bufs[b].handle, quit);
				}
				close(bufs[b].handle);
				FD_CLR(bufs[b].handle, master);
			}
			int b2;
			for(b2=1;b2<nbufs;b2++)
			{
				while((b2<nbufs) && (bufs[b2].type!=SERVER) && ((bufs[b2].server==b) || (bufs[b2].server==0)))
				{
					bufs[b2].live=false;
					free_buffer(b2);
				}
			}
			bufs[b].live=false;
			free_buffer(b);
			cbuf=0;
			if(force_redraw<3) redraw_buffer();
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't disconnect (status)!", "/disconnect: ");
		}
		return(0);
	}
	if(strcmp(cmd, "realsname")==0)
	{
		int b=bufs[cbuf].server;
		if(b>0)
		{
			if(bufs[b].realsname)
				add_to_buffer(cbuf, STA, NORMAL, 0, false, bufs[b].realsname, "/realsname: ");
			else
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "unknown", "/realsname ");
		}
		else
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "(status) is not a server", "/realsname: ");
		return(0);
	}
	if(strcmp(cmd, "join")==0)
	{
		if(!bufs[bufs[cbuf].server].handle)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/join: ");
		}
		else if(!bufs[bufs[cbuf].server].live)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Disconnected, can't send", "/join: ");
		}
		else if(args)
		{
			char *chan=strtok(args, " ");
			if(!chan)
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a channel!", "/join: ");
			}
			else
			{
				char *pass=strtok(NULL, ", ");
				if(!pass) pass="";
				char joinmsg[8+strlen(chan)+strlen(pass)];
				sprintf(joinmsg, "JOIN %s %s", chan, pass);
				irc_tx(bufs[bufs[cbuf].server].handle, joinmsg);
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a channel!", "/join: ");
		}
		return(0);
	}
	if(strcmp(cmd, "rejoin")==0)
	{
		if(bufs[cbuf].type==PRIVATE)
		{
			if(!(bufs[bufs[cbuf].server].handle && bufs[bufs[cbuf].server].live))
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Disconnected, can't send", "/rejoin: ");
			}
			else if(bufs[cbuf].live)
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Already in this channel", "/rejoin: ");
			}
			else
			{
				bufs[cbuf].live=true;
			}
		}
		else if(bufs[cbuf].type!=CHANNEL)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "View is not a channel!", "/rejoin: ");
		}
		else if(!(bufs[bufs[cbuf].server].handle && bufs[bufs[cbuf].server].live))
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Disconnected, can't send", "/rejoin: ");
		}
		else if(bufs[cbuf].live)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Already in this channel", "/rejoin: ");
		}
		else
		{
			char *chan=bufs[cbuf].bname;
			char *pass=args;
			if(!pass) pass="";
			char joinmsg[8+strlen(chan)+strlen(pass)];
			sprintf(joinmsg, "JOIN %s %s", chan, pass);
			irc_tx(bufs[bufs[cbuf].server].handle, joinmsg);
			if(force_redraw<3)
			{
				redraw_buffer();
			}
		}
		return(0);
	}
	if((strcmp(cmd, "part")==0)||(strcmp(cmd, "leave")==0))
	{
		if(bufs[cbuf].type!=CHANNEL)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "This view is not a channel!", "/part: ");
		}
		else
		{
			if(LIVE(cbuf) && bufs[bufs[cbuf].server].handle)
			{
				char partmsg[8+strlen(bufs[cbuf].bname)];
				sprintf(partmsg, "PART %s", bufs[cbuf].bname);
				irc_tx(bufs[bufs[cbuf].server].handle, partmsg);
				add_to_buffer(cbuf, PART, NORMAL, 0, true, "Leaving", "/part: ");
			}
			// when you try to /part a dead tab, interpret it as a /close
			int parent=bufs[cbuf].server;
			bufs[cbuf].live=false;
			free_buffer(cbuf);
			cbuf=parent;
			redraw_buffer();
		}
		return(0);
	}
	if(strcmp(cmd, "unaway")==0)
	{
		cmd="away";
		args="-";
	}
	if(strcmp(cmd, "away")==0)
	{
		const char *am="Gone away, gone away, was it one of you took it away?";
		if(args)
			am=args;
		if(bufs[bufs[cbuf].server].handle)
		{
			if(LIVE(cbuf))
			{
				if(strcmp(am, "-"))
				{
					char nmsg[8+strlen(am)];
					sprintf(nmsg, "AWAY :%s", am);
					irc_tx(bufs[bufs[cbuf].server].handle, nmsg);
				}
				else
				{
					irc_tx(bufs[bufs[cbuf].server].handle, "AWAY"); // unmark away
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/away: ");
			}
		}
		else
		{
			if(cbuf)
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/away: ");
			else
			{
				int b;
				for(b=0;b<nbufs;b++)
				{
					if(bufs[b].type==SERVER)
					{
						if(bufs[b].handle)
						{
							if(LIVE(b))
							{
								if(strcmp(am, "-"))
								{
									char nmsg[8+strlen(am)];
									sprintf(nmsg, "AWAY :%s", am);
									irc_tx(bufs[b].handle, nmsg);
								}
								else
								{
									irc_tx(bufs[b].handle, "AWAY"); // unmark away
								}
							}
							else
								add_to_buffer(b, ERR, NORMAL, 0, false, "Tab not live, can't send", "/away: ");
						}
						else
							add_to_buffer(b, ERR, NORMAL, 0, false, "Tab not live, can't send", "/away: ");
					}
				}
			}
		}
		return(0);
	}
	bool aalloc=false;
	if(strcmp(cmd, "afk")==0)
	{
		const char *afm="afk";
		if(args)
			afm=strtok(args, " ");
		const char *p=bufs[bufs[cbuf].server].nick;
		int n=strcspn(p, "|");
		char *nargs=malloc(n+strlen(afm)+2);
		if(nargs)
		{
			cmd="nick";
			strncpy(nargs, p, n);
			nargs[n]=0;
			if(strcmp(afm, "-"))
			{
				strcat(nargs, "|");
				strcat(nargs, afm);
			}
			args=nargs;
			aalloc=true;
		}
	}
	if(strcmp(cmd, "nick")==0)
	{
		if(args)
		{
			char *nn=strtok(args, " ");
			if(bufs[bufs[cbuf].server].handle)
			{
				if(LIVE(cbuf))
				{
					free(bufs[bufs[cbuf].server].nick);
					bufs[bufs[cbuf].server].nick=strdup(nn);
					char nmsg[8+strlen(bufs[bufs[cbuf].server].nick)];
					sprintf(nmsg, "NICK %s", bufs[bufs[cbuf].server].nick);
					irc_tx(bufs[bufs[cbuf].server].handle, nmsg);
					add_to_buffer(cbuf, STA, QUIET, 0, false, "Changing nick", "/nick: ");
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/nick: ");
				}
			}
			else
			{
				if(cbuf)
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/nick: ");
				else
				{
					free(bufs[0].nick);
					bufs[0].nick=strdup(nn);
					free(nick);
					nick=strdup(nn);
					defnick=false;
					add_to_buffer(cbuf, STA, QUIET, 0, false, "Default nick changed", "/nick: ");
				}
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a nickname!", "/nick: ");
		}
		if(aalloc) free(args);
		return(0);
	}
	if(strcmp(cmd, "topic")==0)
	{
		if(args)
		{
			if(bufs[cbuf].type==CHANNEL)
			{
				if(bufs[bufs[cbuf].server].handle)
				{
					if(LIVE(cbuf))
					{
						char tmsg[10+strlen(bufs[cbuf].bname)+strlen(args)];
						sprintf(tmsg, "TOPIC %s :%s", bufs[cbuf].bname, args);
						irc_tx(bufs[bufs[cbuf].server].handle, tmsg);
					}
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/topic: ");
					}
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't send to channel - not connected!", "/topic: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't set topic - view is not a channel!", "/topic: ");
			}
		}
		else
		{
			if(bufs[cbuf].type==CHANNEL)
			{
				if(bufs[bufs[cbuf].server].handle)
				{
					if(LIVE(cbuf))
					{
						char tmsg[8+strlen(bufs[cbuf].bname)];
						sprintf(tmsg, "TOPIC %s", bufs[cbuf].bname);
						irc_tx(bufs[bufs[cbuf].server].handle, tmsg);
					}
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/topic: ");
					}
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't send to channel - not connected!", "/topic: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't get topic - view is not a channel!", "/topic: ");
			}
		}
		return(0);
	}
	if(strcmp(cmd, "msg")==0)
	{
		if(!bufs[bufs[cbuf].server].handle)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/msg: ");
		}
		else if(args)
		{
			bool no_tab=false;
			char *dest=strtok(args, " ");
			if(strcmp(dest, "-n")==0)
			{
				no_tab=true;
				dest=strtok(NULL, " ");
			}
			if(!dest)
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a recipient!", "/msg: ");
				return(0);
			}
			char *text=strtok(NULL, "");
			if(!no_tab)
			{
				int b2=findptab(bufs[cbuf].server, dest);
				if(b2<0)
				{
					bufs=(buffer *)realloc(bufs, ++nbufs*sizeof(buffer));
					init_buffer(nbufs-1, PRIVATE, dest, buflines);
					b2=nbufs-1;
					bufs[b2].server=bufs[cbuf].server;
					bufs[b2].handle=bufs[bufs[b2].server].handle;
					bufs[b2].live=true;
					if(bufs[bufs[b2].server].nick) n_add(&bufs[b2].nlist, bufs[bufs[b2].server].nick, bufs[bufs[b2].server].casemapping);
					n_add(&bufs[b2].nlist, dest, bufs[bufs[b2].server].casemapping);
				}
				cbuf=b2;
				redraw_buffer();
			}
			if(text)
			{
				if(bufs[bufs[cbuf].server].handle)
				{
					if(LIVE(cbuf))
					{
						char privmsg[12+strlen(dest)+strlen(text)];
						sprintf(privmsg, "PRIVMSG %s :%s", dest, text);
						irc_tx(bufs[bufs[cbuf].server].handle, privmsg);
						if(no_tab)
						{
							add_to_buffer(cbuf, STA, QUIET, 0, false, "sent", "/msg -n: ");
						}
						else
						{
							while(text[strlen(text)-1]=='\n')
								text[strlen(text)-1]=0; // stomp out trailing newlines, they break things
							add_to_buffer(cbuf, MSG, NORMAL, 0, true, text, bufs[bufs[cbuf].server].nick);
						}
					}
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/msg: ");
					}
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't send to channel - not connected!", "/msg: ");
				}
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a recipient!", "/msg: ");
		}
		return(0);
	}
	if(strcmp(cmd, "ping")==0)
	{
		if(!bufs[bufs[cbuf].server].handle)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/ping: ");
		}
		else
		{
			const char *dest=NULL;
			if(args)
				dest=strtok(args, " ");
			if(!dest&&bufs[cbuf].type==PRIVATE)
				dest=bufs[cbuf].bname;
			if(dest)
			{
				if(bufs[bufs[cbuf].server].handle)
				{
					if(LIVE(cbuf))
					{
						struct timeval tv;
						gettimeofday(&tv, NULL);
						char privmsg[64+strlen(dest)];
						snprintf(privmsg, 64+strlen(dest), "PRIVMSG %s :\001PING %u %u\001", dest, (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);
						irc_tx(bufs[bufs[cbuf].server].handle, privmsg);
					}
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/ping: ");
					}
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't send - not connected!", "/ping: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a recipient!", "/ping: ");
			}
		}
		return(0);
	}
	if(strcmp(cmd, "amsg")==0)
	{
		if(!bufs[cbuf].server)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/amsg: ");
		}
		else if(args)
		{
			int b2;
			for(b2=1;b2<nbufs;b2++)
			{
				if((bufs[b2].server==bufs[cbuf].server) && (bufs[b2].type==CHANNEL))
				{
					if(LIVE(b2))
					{
						char privmsg[12+strlen(bufs[b2].bname)+strlen(args)];
						sprintf(privmsg, "PRIVMSG %s :%s", bufs[b2].bname, args);
						irc_tx(bufs[b2].handle, privmsg);
						while(args[strlen(args)-1]=='\n')
							args[strlen(args)-1]=0; // stomp out trailing newlines, they break things
						bool al=bufs[b2].alert; // save alert status...
						int hi=bufs[b2].hi_alert;
						add_to_buffer(b2, MSG, NORMAL, 0, true, args, bufs[bufs[b2].server].nick);
						bufs[b2].alert=al; // and restore it
						bufs[b2].hi_alert=hi;
					}
					else
					{
						add_to_buffer(b2, ERR, NORMAL, 0, false, "Tab not live, can't send", "/amsg: ");
					}
				}
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a message!", "/amsg: ");
		}
		return(0);
	}
	if(strcmp(cmd, "me")==0)
	{
		if(!((bufs[cbuf].type==CHANNEL)||(bufs[cbuf].type==PRIVATE)))
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't talk here, not a channel/private chat", "/me: ");
		}
		else if(args)
		{
			if(bufs[bufs[cbuf].server].handle)
			{
				if(LIVE(cbuf))
				{
					char privmsg[32+strlen(bufs[cbuf].bname)+strlen(args)];
					sprintf(privmsg, "PRIVMSG %s :\001ACTION %s\001", bufs[cbuf].bname, args);
					irc_tx(bufs[bufs[cbuf].server].handle, privmsg);
					while(args[strlen(args)-1]=='\n')
						args[strlen(args)-1]=0; // stomp out trailing newlines, they break things
					add_to_buffer(cbuf, ACT, NORMAL, 0, true, args, bufs[bufs[cbuf].server].nick);
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/me: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't send to channel - not connected!", "/msg: ");
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify an action!", "/me: ");
		}
		return(0);
	}
	if(strcmp(cmd, "tab")==0)
	{
		if(!args)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a tab!", "/tab: ");
		}
		else
		{
			int bufn;
			if(sscanf(args, "%d", &bufn)==1)
			{
				if((bufn>=0) && (bufn<nbufs))
				{
					cbuf=bufn;
					if(force_redraw<3) redraw_buffer();
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "No such tab!", "/tab: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a tab!", "/tab: ");
			}
		}
		return(0);
	}
	if(strcmp(cmd, "sort")==0)
	{
		int newbufs[nbufs];
		buffer bi[nbufs];
		newbufs[0]=0;
		int b,buf=1;
		for(b=0;b<nbufs;b++)
		{
			bi[b]=bufs[b];
			if(bufs[b].type==SERVER)
			{
				newbufs[buf++]=b;
				int b2;
				for(b2=1;b2<nbufs;b2++)
				{
					if((bufs[b2].server==b)&&(b2!=b))
					{
						newbufs[buf++]=b2;
					}
				}
			}
		}
		if(buf!=nbufs)
		{
			add_to_buffer(cbuf, ERR, QUIET, 0, false, "Internal error (bad count)", "/sort: ");
			return(0);
		}
		int serv=0, cb=0;
		for(b=0;b<nbufs;b++)
		{
			bufs[b]=bi[newbufs[b]];
			if(bufs[b].type==SERVER)
				serv=b;
			bufs[b].server=serv;
			if(newbufs[b]==cbuf)
				cb=b;
		}
		cbuf=cb;
		if(force_redraw<3) redraw_buffer();
		return(0);
	}
	if(strcmp(cmd, "left")==0)
	{
		if(cbuf<2)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't move (status) tab!", "/left: ");
		}
		else
		{
			buffer tmp=bufs[cbuf];
			bufs[cbuf]=bufs[cbuf-1];
			bufs[cbuf-1]=tmp;
			cbuf--;
			int i;
			for(i=0;i<nbufs;i++)
			{
				if(bufs[i].server==cbuf)
				{
					bufs[i].server++;
				}
				else if(bufs[i].server==cbuf+1)
				{
					bufs[i].server--;
				}
			}
			if(force_redraw<3) redraw_buffer();
		}
		return(0);
	}
	if(strcmp(cmd, "right")==0)
	{
		if(!cbuf)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't move (status) tab!", "/right: ");
		}
		else if(cbuf==nbufs-1)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Nowhere to move to!", "/right: ");
		}
		else
		{
			buffer tmp=bufs[cbuf];
			bufs[cbuf]=bufs[cbuf+1];
			bufs[cbuf+1]=tmp;
			cbuf++;
			int i;
			for(i=0;i<nbufs;i++)
			{
				if(bufs[i].server==cbuf-1)
				{
					bufs[i].server++;
				}
				else if(bufs[i].server==cbuf)
				{
					bufs[i].server--;
				}
			}
			if(force_redraw<3) redraw_buffer();
		}
		return(0);
	}
	if(strcmp(cmd, "ignore")==0)
	{
		if(!args)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Missing arguments!", "/ignore: ");
		}
		else
		{
			char *arg=strtok(args, " ");
			bool regex=false;
			bool icase=false;
			bool pms=false;
			bool del=false;
			while(arg)
			{
				if(*arg=='-')
				{
					if(strcmp(arg, "-i")==0)
					{
						icase=true;
					}
					else if(strcmp(arg, "-r")==0)
					{
						regex=true;
					}
					else if(strcmp(arg, "-d")==0)
					{
						del=true;
					}
					else if(strcmp(arg, "-p")==0)
					{
						pms=true;
					}
					else if(strcmp(arg, "-l")==0)
					{
						i_list();
						break;
					}
					else if(strcmp(arg, "--")==0)
					{
						arg=strtok(NULL, "");
						continue;
					}
				}
				else
				{
					if(del)
					{
						if(i_cull(&bufs[cbuf].ilist, arg))
						{
							add_to_buffer(cbuf, STA, QUIET, 0, false, "Entries deleted", "/ignore -d: ");
						}
						else
						{
							add_to_buffer(cbuf, ERR, NORMAL, 0, false, "No entries deleted", "/ignore -d: ");
						}
					}
					else if(regex)
					{
						name *new=n_add(&bufs[cbuf].ilist, arg, bufs[cbuf].casemapping);
						if(new)
						{
							add_to_buffer(cbuf, STA, QUIET, 0, false, "Entry added", "/ignore: ");
							new->icase=icase;
							new->pms=pms;
						}
					}
					else
					{
						char *isrc,*iusr,*ihst;
						prefix_split(arg, &isrc, &iusr, &ihst);
						if((!isrc) || (*isrc==0) || (*isrc=='*'))
							isrc="[^!@]*";
						if((!iusr) || (*iusr==0) || (*iusr=='*'))
							iusr="[^!@]*";
						if((!ihst) || (*ihst==0) || (*ihst=='*'))
							ihst="[^@]*";
						char expr[16+strlen(isrc)+strlen(iusr)+strlen(ihst)];
						sprintf(expr, "^%s[_~]*!%s@%s$", isrc, iusr, ihst);
						name *new=n_add(&bufs[cbuf].ilist, expr, bufs[cbuf].casemapping);
						if(new)
						{
							add_to_buffer(cbuf, STA, QUIET, 0, false, "Entry added", "/ignore: ");
							new->icase=icase;
							new->pms=pms;
						}
					}
					break;
				}
				arg=strtok(NULL, " ");
			}
		}
		return(0);
	}
	if(strcmp(cmd, "mode")==0)
	{
		if(!bufs[bufs[cbuf].server].handle)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/mode: ");
		}
		else
		{
			// /mode [{<user>|<banmask>|<limit>} [{\+|-}[[:alpha:]]+]]
			if(args)
			{
				char *user=strtok(args, " ");
				char *modes=strtok(NULL, "");
				if(modes)
				{
					if(LIVE(cbuf))
					{
						if(bufs[cbuf].type==CHANNEL)
						{
							char mmsg[8+strlen(bufs[cbuf].bname)+strlen(modes)+strlen(user)];
							sprintf(mmsg, "MODE %s %s %s", bufs[cbuf].bname, modes, user);
							irc_tx(bufs[bufs[cbuf].server].handle, mmsg);
						}
						else
							add_to_buffer(cbuf, ERR, NORMAL, 0, false, "This is not a channel", "/mode: ");
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/mode: ");
				}
				else
				{
					if(bufs[cbuf].type==CHANNEL)
					{
						name *curr=bufs[cbuf].nlist;
						while(curr)
						{
							if(irc_strcasecmp(curr->data, user, bufs[bufs[cbuf].server].casemapping)==0)
							{
								if(curr==bufs[cbuf].us) goto youmode;
								char mm[12+strlen(curr->data)+curr->npfx];
								int mpos=0;
								sprintf(mm, "%s has mode %n-", curr->data, &mpos);
								if(mpos)
								{
									for(unsigned int i=0;i<curr->npfx;i++)
										mm[mpos++]=curr->prefixes[i].letter;
									if(curr->npfx) mm[mpos]=0;
									add_to_buffer(cbuf, MODE, NORMAL, 0, false, mm, "/mode: ");
								}
								else
									add_to_buffer(cbuf, ERR, NORMAL, 0, false, "\"Impossible\" error (mpos==0)", "/mode: ");
								break;
							}
							curr=curr->next;
						}
						if(!curr)
						{
							char mm[16+strlen(user)];
							sprintf(mm, "No such nick: %s", user);
							add_to_buffer(cbuf, ERR, NORMAL, 0, false, mm, "/mode: ");
						}
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "This is not a channel", "/mode: ");
				}
			}
			else
			{
				youmode:
				if(bufs[cbuf].type==CHANNEL)
				{
					if(bufs[cbuf].us)
					{
						char mm[20+strlen(bufs[bufs[cbuf].server].nick)+bufs[cbuf].us->npfx];
						int mpos=0;
						sprintf(mm, "You (%s) have mode %n-", bufs[bufs[cbuf].server].nick, &mpos);
						if(mpos)
						{
							for(unsigned int i=0;i<bufs[cbuf].us->npfx;i++)
								mm[mpos++]=bufs[cbuf].us->prefixes[i].letter;
							if(bufs[cbuf].us->npfx) mm[mpos]=0;
							add_to_buffer(cbuf, MODE, NORMAL, 0, true, mm, "/mode: ");
						}
						else
							add_to_buffer(cbuf, ERR, NORMAL, 0, false, "\"Impossible\" error (mpos==0)", "/mode: ");
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "\"Impossible\" error (us==NULL)", "/mode: ");
				}
				else
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "This is not a channel", "/mode: ");
			}
		}
		return(0);
	}
	if(strcmp(cmd, "cmd")==0)
	{
		if(!bufs[bufs[cbuf].server].handle)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/cmd: ");
		}
		else
		{
			bool force=false;
			if(strncmp(args, "-f", 2)==0)
			{
				force=true;
				args++;
				while (*++args==' ');
			}
			if(force||LIVE(cbuf))
			{
				irc_tx(bufs[bufs[cbuf].server].handle, args);
				add_to_buffer(cbuf, STA, NORMAL, 0, false, args, "/cmd: ");
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/cmd: ");
			}
		}
		return(0);
	}
	if(!cmd) cmd="";
	char dstr[8+strlen(cmd)];
	sprintf(dstr, "/%s: ", cmd);
	add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Unrecognised command!", dstr);
	return(0);
}

void initibuf(ibuffer *i)
{
	i->nlines=buflines;
	i->ptr=0;
	i->scroll=0;
	i->filled=false;
	i->line=(char **)malloc(i->nlines*sizeof(char *));
}

void addtoibuf(ibuffer *i, char *data)
{
	if(i)
	{
		if(i->filled)
		{
			if(i->line[i->ptr])
				free(i->line[i->ptr]);
		}
		i->line[i->ptr++]=strdup(data?data:"");
		if(i->ptr>=i->nlines)
		{
			i->ptr=0;
			i->filled=true;
		}
		i->scroll=0;
	}
}

void freeibuf(ibuffer *i)
{
	if(i->line)
	{
		int l;
		for(l=0;l<(i->filled?i->nlines:i->ptr);l++)
		{
			if(i->line[l])
				free(i->line[l]);
		}
		free(i->line);
	}
}

char back_ichar(ichar *buf)
{
	char c=0;
	if(buf->i)
	{
		c=buf->data[--(buf->i)];
		buf->data[(buf->i)]=0;
	}
	return(c);
}

void ifree(iline *buf)
{
	free(buf->left.data);
	free(buf->right.data);
	buf->left.data=NULL;
	buf->right.data=NULL;
	buf->left.i=buf->left.l=0;
	buf->right.i=buf->right.l=0;
}

void i_home(iline *inp)
{
	if(inp->left.i)
	{
		size_t b=inp->left.i+inp->right.i;
		char *nr=(char *)malloc(b+1);
		sprintf(nr, "%s%s", inp->left.data?inp->left.data:"", inp->right.data?inp->right.data:"");
		ifree(inp);
		inp->right.data=nr;
		inp->right.i=b;
		inp->right.l=b+1;
	}
}

void i_end(iline *inp)
{
	if(inp->right.i)
	{
		size_t b=inp->left.i+inp->right.i;
		char *nl=(char *)malloc(b+1);
		sprintf(nl, "%s%s", inp->left.data?inp->left.data:"", inp->right.data?inp->right.data:"");
		ifree(inp);
		inp->left.data=nl;
		inp->left.i=b;
		inp->left.l=b+1;
	}
}
