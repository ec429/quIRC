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
	else if((c<32)||(c>127)) // this also stomps on the newline 
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
							buf_print(cbuf, c_err, curr->data, true);
						}
						else if(found)
						{
							buf_print(cbuf, c_err, "[tab] Multiple nicks match", true);
							buf_print(cbuf, c_err, found->data, true);
							buf_print(cbuf, c_err, curr->data, true);
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
					buf_print(cbuf, c_err, "[tab] No nicks match", true);
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

int cmd_handle(char *inp, char **qmsg, fd_set *master, int *fdmax) // old state=3; return new state
{
	char *cmd=inp+1;
	char *args=strchr(cmd, ' ');
	if(args) *args++=0;
	if((strcmp(cmd, "quit")==0)||(strcmp(cmd, "exit")==0))
	{
		if(args) *qmsg=args;
		printf(LOCATE, height-2, 1);
		printf(CLA "Exited quirc\n" CLA "\n");
		return(-1);
	}
	else if(strcmp(cmd, "set")==0) // set options
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
						buf_print(cbuf, c_status, "width set to minimum 30", false);
						width=30;
					}
					else
					{
						buf_print(cbuf, c_status, "width set", false);
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
						buf_print(cbuf, c_status, "height set to minimum 5", false);
						height=5;
					}
					else
					{
						buf_print(cbuf, c_status, "height set", false);
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
						w_buf_print(cbuf, c_status, fmsg, false, "");
						redraw_buffer();
					}
					else
					{
						buf_print(cbuf, c_status, "force-redraw disabled", false);
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
					buf_print(cbuf, c_status, "maxnicklen set", false);
					if(force_redraw<3)
					{
						redraw_buffer();
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
					buf_print(cbuf, c_status, "mcc set", false);
					if(force_redraw<3)
					{
						redraw_buffer();
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
					buf_print(cbuf, c_status, "buf set", false);
					if(force_redraw<3)
					{
						redraw_buffer();
					}
				}
				else
				{
					buf_print(cbuf, c_err, "set: No such option!", false);
				}
			}
			else
			{
				buf_print(cbuf, c_err, "set what?", false);
			}
		}
		else
		{
			buf_print(cbuf, c_err, "set what?", false);
		}
		return(0);
	}
	else if(strcmp(cmd, "server")==0)
	{
		if(args)
		{
			server=strdup(args);
			char *newport=strchr(server, ':');
			if(newport)
			{
				*newport=0;
				portno=newport+1;
			}
			char cstr[24+strlen(server)];
			sprintf(cstr, "quIRC - connecting to %s", server);
			settitle(cstr);
			char dstr[30+strlen(server)+strlen(portno)];
			sprintf(dstr, "Connecting to %s on port %s...", server, portno);
			setcolour(c_status);
			printf(LOCATE, height-2, 1);
			printf("%s" CLR "\n", dstr);
			resetcol();
			printf(CLA "\n");
			int serverhandle=irc_connect(server, portno, nick, username, fname, master, fdmax);
			if(serverhandle)
			{
				bufs=(buffer *)realloc(bufs, ++nbufs*sizeof(buffer));
				init_buffer(nbufs-1, SERVER, server, buflines);
				cbuf=nbufs-1;
				bufs[cbuf].handle=serverhandle;
				bufs[cbuf].nick=strdup(nick);
				bufs[cbuf].server=cbuf;
				w_buf_print(cbuf, c_status, dstr, false, "");
				sprintf(cstr, "quIRC - connected to %s", server);
				settitle(cstr);
			}
		}
		else
		{
			buf_print(cbuf, c_err, "Must specify a server!", false);
		}
		return(0);
	}
	else if(strcmp(cmd, "disconnect")==0)
	{
		int b=bufs[cbuf].server;
		if(b>0)
		{
			buf_print(cbuf, c_status, "Disconnecting...", false);
			close(bufs[b].handle);
			FD_CLR(bufs[b].handle, master);
			int b2;
			for(b2=1;b2<nbufs;b2++)
			{
				while((b2<nbufs) && ((bufs[b2].server==b) || (bufs[b2].server==0)))
				{
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
			buf_print(cbuf, c_err, "Can't disconnect (status)!", false);
		}
		return(0);
	}
	else if(strcmp(cmd, "join")==0)
	{
		if(!bufs[cbuf].handle)
		{
			w_buf_print(cbuf, c_err, "/join: must be run in the context of a server!", false, "");
		}
		else if(args)
		{
			char *chan=strtok(args, " ");
			char *pass=strtok(NULL, ", ");
			if(!pass) pass="";
			char joinmsg[8+strlen(chan)+strlen(pass)];
			sprintf(joinmsg, "JOIN %s %s", chan, pass);
			irc_tx(bufs[cbuf].handle, joinmsg);
			setcolour(c_join[0]);
			printf(LOCATE, height-2, 1);
			printf("Joining" CLR "\n");
			resetcol();
			printf(CLA "\n");
		}
		else
		{
			buf_print(cbuf, c_err, "Must specify a channel!", false);
		}
		return(0);
	}
	else if((strcmp(cmd, "part")==0)||(strcmp(cmd, "leave")==0))
	{
		if(bufs[cbuf].type!=CHANNEL)
		{
			w_buf_print(cbuf, c_err, "/part: This view is not a channel!", false, "");
		}
		else
		{
			char partmsg[8+strlen(bufs[cbuf].bname)];
			sprintf(partmsg, "PART %s", bufs[cbuf].bname);
			irc_tx(bufs[cbuf].handle, partmsg);
			buf_print(cbuf, c_part[0], "Leaving", false);
		}
		return(0);
	}
	else if(strcmp(cmd, "nick")==0)
	{
		if(args)
		{
			char *nn=strtok(args, " ");
			if(bufs[cbuf].handle)
			{
				bufs[bufs[cbuf].server].nick=strdup(nn);
				char nmsg[8+strlen(bufs[bufs[cbuf].server].nick)];
				sprintf(nmsg, "NICK %s", bufs[bufs[cbuf].server].nick);
				irc_tx(bufs[cbuf].handle, nmsg);
				buf_print(cbuf, c_status, "Changing nick", false);
			}
			else
			{
				nick=strdup(nn);
				buf_print(cbuf, c_status, "Default nick changed", false);
			}
		}
		else
		{
			buf_print(cbuf, c_err, "Must specify a nickname!", false);
		}
		return(0);
	}
	else if(strcmp(cmd, "msg")==0)
	{
		if(!bufs[cbuf].handle)
		{
			w_buf_print(cbuf, c_err, "/msg: must be run in the context of a server!", false, "");
		}
		else if(args)
		{
			char *dest=strtok(args, " ");
			char *text=strtok(NULL, "");
			if(text)
			{
				char privmsg[12+strlen(dest)+strlen(text)];
				sprintf(privmsg, "PRIVMSG %s :%s", dest, text);
				irc_tx(bufs[cbuf].handle, privmsg);
				while(text[strlen(text)-1]=='\n')
					text[strlen(text)-1]=0; // stomp out trailing newlines, they break things
				char *out=(char *)malloc(16+max(maxnlen, strlen(dest)));
				memset(out, ' ', 2+max(maxnlen-strlen(dest), 0));
				out[2+max(maxnlen-strlen(dest), 0)]=0;
				sprintf(out+strlen(out), "(to %s) ", dest);
				wordline(text, 9+max(maxnlen, strlen(dest)), &out);
				buf_print(cbuf, c_msg[0], out, false);
				free(out);
			}
			else
			{
				w_buf_print(cbuf, c_err, "/msg: must specify a message!", false, "");
			}
		}
		else
		{
				w_buf_print(cbuf, c_err, "/msg: must specify a recipient!", false, "");
		}
		return(0);
	}
	else if(strcmp(cmd, "me")==0)
	{
		if(bufs[cbuf].type!=CHANNEL) // TODO add PRIVATE
		{
			w_buf_print(cbuf, c_err, "/me: this view is not a channel!", false, "");
		}
		else if(args)
		{
			char privmsg[32+strlen(bufs[cbuf].bname)+strlen(args)];
			sprintf(privmsg, "PRIVMSG %s :\001ACTION %s\001", bufs[cbuf].bname, args);
			irc_tx(bufs[cbuf].handle, privmsg);
			while(args[strlen(args)-1]=='\n')
				args[strlen(args)-1]=0; // stomp out trailing newlines, they break things
			char *out=(char *)malloc(8+max(maxnlen, strlen(bufs[bufs[cbuf].server].nick)));
			memset(out, ' ', 2+max(maxnlen-strlen(bufs[bufs[cbuf].server].nick), 0));
			out[2+max(maxnlen-strlen(bufs[bufs[cbuf].server].nick), 0)]=0;
			sprintf(out+strlen(out), "%s ", bufs[bufs[cbuf].server].nick);
			wordline(args, 3+max(maxnlen, strlen(bufs[bufs[cbuf].server].nick)), &out);
			buf_print(cbuf, c_actn[0], out, false);
			free(out);
		}
		else
		{
			w_buf_print(cbuf, c_err, "/me: must specify an action!", false, "");
		}
		return(0);
	}
	else if(strcmp(cmd, "cmd")==0)
	{
		if(!bufs[cbuf].handle)
		{
			w_buf_print(cbuf, c_err, "/cmd: must be run in the context of a server!", false, "");
		}
		else
		{
			irc_tx(bufs[cbuf].handle, args);
		}
		return(0);
	}
	else
	{
		if(!cmd) cmd="";
		char dstr[30+strlen(cmd)];
		sprintf(dstr, "%s: Unrecognised command!", cmd);
		w_buf_print(cbuf, c_err, dstr, false, "");
		return(0);
	}
}
