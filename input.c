/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	input: handle input routines
*/

#include "input.h"

int inputchar(char **inp, int *state)
{
	printf("\010\010\010" CLA);
	int ino=(*inp)?strlen(*inp):0;
	*inp=(char *)realloc(*inp, ino+2);
	unsigned char c=(*inp)[ino]=getchar();
	(*inp)[ino+1]=0;
	if(strchr("\010\177", c)) // various backspace-type characters
	{
		if(ino)
			(*inp)[ino-1]=0;
		(*inp)[ino]=0;
	}
	else if((c<32) || (c==0xc2)) // this also stomps on the newline
	{
		(*inp)[ino]=0;
		if(c==1)
		{
			free(*inp);
			*inp=NULL;
			ino=0;
		}
		if(ino>0)
		{
			if(c=='\t') // tab completion of nicks
			{
				int sp=ino-1;
				while(sp>0 && !strchr(" \t", (*inp)[sp-1]))
					sp--;
				name *curr=bufs[cbuf].nlist;
				name *found=NULL;bool tmany=false;
				while(curr)
				{
					if(strncasecmp((*inp)+sp, curr->data, ino-sp)==0)
					{
						if(tmany)
						{
							w_buf_print(cbuf, c_err, curr->data, "[tab] ");
						}
						else if(found)
						{
							w_buf_print(cbuf, c_err, "Multiple nicks match", "[tab] ");
							w_buf_print(cbuf, c_err, found->data, "[tab] ");
							w_buf_print(cbuf, c_err, curr->data, "[tab] ");
							found=NULL;tmany=true;
						}
						else
							found=curr;
					}
					if(curr)
						curr=curr->next;
				}
				if(found)
				{
					*inp=(char *)realloc(*inp, sp+strlen(found->data)+4);
					if(sp)
						sprintf((*inp)+sp, "%s", found->data);
					else
						sprintf((*inp)+sp, "%s: ", found->data);
				}
				else if(!tmany)
				{
					w_buf_print(cbuf, c_err, "No nicks match", "[tab] ");
				}
			}
		}
	}
	if(c=='\033') // escape sequence
	{
		if(getchar()=='\133') // 1b 5b
		{
			unsigned char d=getchar();
			switch(d)
			{
				case 'D': // left cursor counts as a backspace
					if(ino)
						(*inp)[ino-1]=0;
				break;
				case '3': // take another
					if(getchar()=='~') // delete
					{
						if(ino)
							(*inp)[ino-1]=0;
					}
				break;
				case '5': // ^[[5
				case '6': // ^[[6
					if(getchar()==';')
					{
						if(getchar()=='5')
						{
							if(getchar()=='~')
							{
								if(d=='5') // C-PgUp
								{
									bufs[cbuf].scroll=min(bufs[cbuf].scroll+height-2, bufs[cbuf].filled?bufs[cbuf].nlines-1:bufs[cbuf].ptr-1);
									redraw_buffer();
								}
								else // d=='6' // C-PgDn
								{
									if(bufs[cbuf].scroll)
									{
										bufs[cbuf].scroll=max(bufs[cbuf].scroll-(height-2), 0);
										redraw_buffer();
									}
								}
							}
						}
					}
				break;
				case '1': // ^[[1
					if(getchar()==';')
					{
						if(getchar()=='5')
						{
							switch(getchar())
							{
								case 'D': // C-left
									cbuf=max(cbuf-1, 0);
									redraw_buffer();
								break;
								case 'C': // C-right
									cbuf=min(cbuf+1, nbufs-1);
									redraw_buffer();
								break;
								case 'A': // C-up
									bufs[cbuf].scroll=min(bufs[cbuf].scroll+1, bufs[cbuf].filled?bufs[cbuf].nlines-1:bufs[cbuf].ptr-1);
									redraw_buffer();
								break;
								case 'B': // C-down
									if(bufs[cbuf].scroll)
									{
										bufs[cbuf].scroll=bufs[cbuf].scroll-1;
										redraw_buffer();
									}
								break;
								case 'F': // C-end
									if(bufs[cbuf].scroll)
									{
										bufs[cbuf].scroll=0;
										redraw_buffer();
									}
								break;
								case 'H': // C-home
									bufs[cbuf].scroll=bufs[cbuf].filled?bufs[cbuf].nlines-1:bufs[cbuf].ptr-1;
									redraw_buffer();
								break;
							}
							
						}
					}
				break;
			}
		}
	}
	else if(c==0xc2) // c2 bN = alt-N (for N in 0...9)
	{
		unsigned char d=getchar();
		if((d&0xf0)==0xb0)
		{
			cbuf=min(max(d&0x0f, 0), nbufs-1);
			redraw_buffer();
		}
	}
	if(c=='\n')
	{
		*state=3;
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
	int o=0;
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
		if(args) *qmsg=strdup(args);
		printf(LOCATE, height-2, 1);
		printf(CLA "Exited quirc\n" CLA "\n");
		return(-1);
	}
	if(strcmp(cmd, "set")==0) // set options
	{
		if(args)
		{
			char *opt=strtok(args, " ");
			if(opt)
			{
				char *val=strtok(NULL, " ");
				if(strcmp(opt, "width")==0)
				{
					if(val)
					{
						sscanf(val, "%u", &width);
					}
					else
					{
						width=80;
					}
					if(width<30)
					{
						w_buf_print(cbuf, c_status, "width set to minimum 30", "/set: ");
						width=30;
					}
					else
					{
						w_buf_print(cbuf, c_status, "width set", "/set: ");
					}
					if(force_redraw<3)
					{
						redraw_buffer();
					}
				}
				else if(strcmp(opt, "height")==0)
				{
					if(val)
					{
						sscanf(val, "%u", &height);
					}
					else
					{
						height=24;
					}
					if(height<5)
					{
						w_buf_print(cbuf, c_status, "height set to minimum 5", "/set: ");
						height=5;
					}
					else
					{
						w_buf_print(cbuf, c_status, "height set", "/set: ");
					}
					if(force_redraw<3)
					{
						redraw_buffer();
					}
				}
				else if(strcmp(opt, "fred")==0)
				{
					if(val)
					{
						sscanf(val, "%u", &force_redraw);
					}
					else
					{
						force_redraw=1;
					}
					if(force_redraw)
					{
						char fmsg[36];
						sprintf(fmsg, "force-redraw level %u enabled", force_redraw);
						w_buf_print(cbuf, c_status, fmsg, "/set: ");
						redraw_buffer();
					}
					else
					{
						w_buf_print(cbuf, c_status, "force-redraw disabled", "/set: ");
					}
				}
				else if(strcmp(opt, "mnln")==0)
				{
					if(val)
					{
						sscanf(val, "%u", &maxnlen);
					}
					else
					{
						maxnlen=16;
					}
					if(maxnlen<4)
					{
						maxnlen=4;
						w_buf_print(cbuf, c_status, "maxnicklen set to minimum 4", "/set: ");
					}
					else
					{
						w_buf_print(cbuf, c_status, "maxnicklen set", "/set: ");
					}
				}
				else if(strcmp(opt, "mcc")==0)
				{
					if(val)
					{
						sscanf(val, "%u", &mirc_colour_compat);
					}
					else
					{
						mirc_colour_compat=1;
					}
					if(mirc_colour_compat)
					{
						char mmsg[26];
						sprintf(mmsg, "mcc level %u enabled", mirc_colour_compat);
						w_buf_print(cbuf, c_status, mmsg, "/set: ");
					}
					else
					{
						w_buf_print(cbuf, c_status, "mcc disabled", "/set: ");
					}
				}
				else if(strcmp(opt, "buf")==0)
				{
					if(val)
					{
						sscanf(val, "%u", &buflines);
					}
					else
					{
						buflines=256;
					}
					w_buf_print(0, c_status, "Default buf set", "/set: ");
				}
				else
				{
					w_buf_print(cbuf, c_err, "No such option!", "/set: ");
				}
			}
			else
			{
				w_buf_print(cbuf, c_err, "But what do you want to set?", "/set: ");
			}
		}
		else
		{
			w_buf_print(cbuf, c_err, "But what do you want to set?", "/set: ");
		}
		return(0);
	}
	if((strcmp(cmd, "server")==0)||(strcmp(cmd, "connect")==0))
	{
		if(args)
		{
			char *server=strdup(args);
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
				if((bufs[b].type==SERVER) && (strcasecmp(server, bufs[b].bname)==0))
				{
					if(bufs[b].live)
					{
						cbuf=b;
						return(0);
					}
					else
					{
						cbuf=b;
						cmd="reconnect";
						break;
					}
				}
			}
			if(b>=nbufs)
			{
				char cstr[24+strlen(server)];
				sprintf(cstr, "quIRC - connecting to %s", server);
				settitle(cstr);
				char dstr[30+strlen(server)+strlen(newport)];
				sprintf(dstr, "Connecting to %s on port %s...", server, newport);
				setcolour(c_status);
				printf(LOCATE, height-2, 1);
				printf("%s" CLR "\n", dstr);
				resetcol();
				printf(CLA "\n");
				int serverhandle=irc_connect(server, newport, master, fdmax);
				if(serverhandle)
				{
					bufs=(buffer *)realloc(bufs, ++nbufs*sizeof(buffer));
					init_buffer(nbufs-1, SERVER, server, buflines);
					cbuf=nbufs-1;
					bufs[cbuf].handle=serverhandle;
					bufs[cbuf].nick=strdup(nick);
					bufs[cbuf].server=cbuf;
					w_buf_print(cbuf, c_status, dstr, "/server: ");
					sprintf(cstr, "quIRC - connected to %s", server);
					settitle(cstr);
				}
			}
		}
		else
		{
			w_buf_print(cbuf, c_err, "Must specify a server!", "/server: ");
		}
		return(0);
	}
	if(strcmp(cmd, "reconnect")==0)
	{
		if(bufs[cbuf].server)
		{
			if(bufs[bufs[cbuf].server].live)
			{
				char *newport;
				if(args)
				{
					newport=args;
				}
				else
				{
					newport=portno;
				}
				char cstr[24+strlen(server)];
				sprintf(cstr, "quIRC - connecting to %s", server);
				settitle(cstr);
				char dstr[30+strlen(server)+strlen(newport)];
				sprintf(dstr, "Connecting to %s on port %s...", server, newport);
				setcolour(c_status);
				printf(LOCATE, height-2, 1);
				printf("%s" CLR "\n", dstr);
				resetcol();
				printf(CLA "\n");
				int serverhandle=irc_connect(server, newport, master, fdmax);
				if(serverhandle)
				{
					int b=bufs[cbuf].server;
					int b2;
					for(b2=1;b2<nbufs;b2++)
					{
						if(bufs[b2].server==b)
							bufs[b2].handle=serverhandle;
					}
					w_buf_print(cbuf, c_status, dstr, "/server: ");
					sprintf(cstr, "quIRC - connected to %s", server);
					settitle(cstr);
				}
			}
			else
			{
				w_buf_print(cbuf, c_err, "Already connected to server", "/reconnect: ");
			}
		}
		else
		{
			w_buf_print(cbuf, c_err, "Must be run in the context of a server!", "/reconnect: ");
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
				while((b2<nbufs) && ((bufs[b2].server==b) || (bufs[b2].server==0)))
				{
					bufs[b2].live=false;
					free_buffer(b2);
				}
			}
			cbuf=0;
			if(force_redraw<3)
			{
				redraw_buffer();
			}
		}
		else
		{
			w_buf_print(cbuf, c_err, "Can't disconnect (status)!", "/disconnect: ");
		}
		return(0);
	}
	if(strcmp(cmd, "join")==0)
	{
		if(!bufs[cbuf].handle)
		{
			w_buf_print(cbuf, c_err, "Must be run in the context of a server!", "/join: ");
		}
		else if(!bufs[bufs[cbuf].server].live)
		{
			w_buf_print(cbuf, c_err, "Disconnected, can't send", "/join: ");
		}
		else if(args)
		{
			char *chan=strtok(args, " ");
			char *pass=strtok(NULL, ", ");
			if(!pass) pass="";
			char joinmsg[8+strlen(chan)+strlen(pass)];
			sprintf(joinmsg, "JOIN %s %s", chan, pass);
			irc_tx(bufs[cbuf].handle, joinmsg);
			if(force_redraw<3)
			{
				redraw_buffer();
			}
		}
		else
		{
			w_buf_print(cbuf, c_err, "Must specify a channel!", "/join: ");
		}
		return(0);
	}
	if(strcmp(cmd, "rejoin")==0)
	{
		if(!bufs[cbuf].handle)
		{
			w_buf_print(cbuf, c_err, "Must be run in the context of a server!", "/rejoin: ");
		}
		else if(!bufs[bufs[cbuf].server].live)
		{
			w_buf_print(cbuf, c_err, "Disconnected, can't send", "/rejoin: ");
		}
		else if(bufs[cbuf].live)
		{
			w_buf_print(cbuf, c_err, "Already in this channel", "/rejoin: ");
		}
		else if(bufs[cbuf].type==CHANNEL)
		{
			char *chan=bufs[cbuf].bname;
			char *pass=args;
			if(!pass) pass="";
			char joinmsg[8+strlen(chan)+strlen(pass)];
			sprintf(joinmsg, "JOIN %s %s", chan, pass);
			irc_tx(bufs[cbuf].handle, joinmsg);
			if(force_redraw<3)
			{
				redraw_buffer();
			}
		}
		else
		{
			w_buf_print(cbuf, c_err, "View is not a channel!", "/rejoin: ");
		}
		return(0);
	}
	if((strcmp(cmd, "part")==0)||(strcmp(cmd, "leave")==0))
	{
		if(bufs[cbuf].type!=CHANNEL)
		{
			w_buf_print(cbuf, c_err, "This view is not a channel!", "/part: ");
		}
		else
		{
			if(LIVE(cbuf))
			{
				char partmsg[8+strlen(bufs[cbuf].bname)];
				sprintf(partmsg, "PART %s", bufs[cbuf].bname);
				irc_tx(bufs[cbuf].handle, partmsg);
				w_buf_print(cbuf, c_part[0], "Leaving", "/part: ");
			}
			// when you try to /part a dead tab, interpret it as a /close
			int parent=bufs[cbuf].server;
			bufs[cbuf].live=false;
			free_buffer(cbuf);
			cbuf=parent;
			char cstr[24+strlen(bufs[cbuf].bname)];
			sprintf(cstr, "quIRC - connected to %s", bufs[cbuf].bname);
			settitle(cstr);
		}
		return(0);
	}
	if(strcmp(cmd, "nick")==0)
	{
		if(args)
		{
			char *nn=strtok(args, " ");
			if(bufs[cbuf].handle)
			{
				if(LIVE(cbuf))
				{
					bufs[bufs[cbuf].server].nick=strdup(nn);
					char nmsg[8+strlen(bufs[bufs[cbuf].server].nick)];
					sprintf(nmsg, "NICK %s", bufs[bufs[cbuf].server].nick);
					irc_tx(bufs[cbuf].handle, nmsg);
					w_buf_print(cbuf, c_status, "Changing nick", "/nick: ");
				}
				else
				{
					w_buf_print(cbuf, c_err, "Tab not live, can't send", "/nick: ");
				}
			}
			else
			{
				nick=strdup(nn);
				w_buf_print(cbuf, c_status, "Default nick changed", "/nick ");
			}
		}
		else
		{
			w_buf_print(cbuf, c_err, "Must specify a nickname!", "/nick: ");
		}
		return(0);
	}
	if(strcmp(cmd, "topic")==0)
	{
		if(args)
		{
			if(bufs[cbuf].type==CHANNEL)
			{
				if(bufs[cbuf].handle)
				{
					if(LIVE(cbuf))
					{
						char tmsg[10+strlen(bufs[cbuf].bname)+strlen(args)];
						sprintf(tmsg, "TOPIC %s :%s", bufs[cbuf].bname, args);
						irc_tx(bufs[cbuf].handle, tmsg);
					}
					else
					{
						w_buf_print(cbuf, c_err, "Tab not live, can't send", "/topic: ");
					}
				}
				else
				{
					w_buf_print(cbuf, c_err, "Can't send to channel - not connected!", "/topic: ");
				}
			}
			else
			{
				w_buf_print(cbuf, c_err, "Can't set topic - view is not a channel!", "/topic: ");
			}
		}
		else
		{
			if(bufs[cbuf].type==CHANNEL)
			{
				if(bufs[cbuf].handle)
				{
					if(LIVE(cbuf))
					{
						char tmsg[8+strlen(bufs[cbuf].bname)];
						sprintf(tmsg, "TOPIC %s", bufs[cbuf].bname);
						irc_tx(bufs[cbuf].handle, tmsg);
					}
					else
					{
						w_buf_print(cbuf, c_err, "Tab not live, can't send", "/topic: ");
					}
				}
				else
				{
					w_buf_print(cbuf, c_err, "Can't send to channel - not connected!", "/topic: ");
				}
			}
			else
			{
				w_buf_print(cbuf, c_err, "Can't get topic - view is not a channel!", "/topic: ");
			}
		}
		return(0);
	}
	if(strcmp(cmd, "msg")==0)
	{
		if(!bufs[cbuf].handle)
		{
			w_buf_print(cbuf, c_err, "Must be run in the context of a server!", "/msg: ");
		}
		else if(args)
		{
			char *dest=strtok(args, " ");
			char *text=strtok(NULL, "");
			if(text)
			{
				if(bufs[cbuf].handle)
				{
					if(LIVE(cbuf))
					{
						char privmsg[12+strlen(dest)+strlen(text)];
						sprintf(privmsg, "PRIVMSG %s :%s", dest, text);
						irc_tx(bufs[cbuf].handle, privmsg);
						while(text[strlen(text)-1]=='\n')
							text[strlen(text)-1]=0; // stomp out trailing newlines, they break things
						char tag[maxnlen+9];
						memset(tag, ' ', maxnlen+8);
						sprintf(tag+maxnlen+2-strlen(dest), "(to %s) ", dest);
						w_buf_print(cbuf, c_msg[0], text, tag);
					}
					else
					{
						w_buf_print(cbuf, c_err, "Tab not live, can't send", "/msg: ");
					}
				}
				else
				{
					w_buf_print(cbuf, c_err, "Can't send to channel - not connected!", "/msg: ");
				}
			}
			else
			{
				w_buf_print(cbuf, c_err, "Must specify a message!", "/msg: ");
			}
		}
		else
		{
			w_buf_print(cbuf, c_err, "Must specify a recipient!", "/msg: ");
		}
		return(0);
	}
	if(strcmp(cmd, "me")==0)
	{
		if(bufs[cbuf].type!=CHANNEL) // TODO add PRIVATE
		{
			w_buf_print(cbuf, c_err, "This view is not a channel!", "/me: ");
		}
		else if(args)
		{
			if(bufs[cbuf].handle)
			{
				if(LIVE(cbuf))
				{
					char privmsg[32+strlen(bufs[cbuf].bname)+strlen(args)];
					sprintf(privmsg, "PRIVMSG %s :\001ACTION %s\001", bufs[cbuf].bname, args);
					irc_tx(bufs[cbuf].handle, privmsg);
					while(args[strlen(args)-1]=='\n')
						args[strlen(args)-1]=0; // stomp out trailing newlines, they break things
					char tag[maxnlen+4];
					memset(tag, ' ', maxnlen+3);
					sprintf(tag+maxnlen+2-strlen(bufs[bufs[cbuf].server].nick), "%s ", bufs[bufs[cbuf].server].nick);
					w_buf_print(cbuf, c_actn[0], args, tag);
				}
				else
				{
					w_buf_print(cbuf, c_err, "Tab not live, can't send", "/me: ");
				}
			}
			else
			{
				w_buf_print(cbuf, c_err, "Can't send to channel - not connected!", "/msg: ");
			}
		}
		else
		{
			w_buf_print(cbuf, c_err, "must specify an action!", "/me: ");
		}
		return(0);
	}
	if(strcmp(cmd, "cmd")==0)
	{
		if(!bufs[cbuf].handle)
		{
			w_buf_print(cbuf, c_err, "must be run in the context of a server!", "/cmd: ");
		}
		else
		{
			if(LIVE(cbuf))
			{
				irc_tx(bufs[cbuf].handle, args);
				w_buf_print(cbuf, c_status, args, "/cmd: ");
			}
			else
			{
				w_buf_print(cbuf, c_err, "Tab not live, can't send", "/cmd: ");
			}
		}
		return(0);
	}
	if(!cmd) cmd="";
	char dstr[8+strlen(cmd)];
	sprintf(dstr, "/%s: ", cmd);
	w_buf_print(cbuf, c_err, "Unrecognised command!", dstr);
	return(0);
}
