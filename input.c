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
	size_t l=strlen(inp);
	char *rv=(char *)malloc(l+1); // we only get shorter, so this will be enough
	int o=0;
	while((*inp) && (o<=l)) // o>l should never happen, but it's covered just in case
	{
		if(*inp=='\\') // \n, \r, \\, \ooo (\0 remains escaped)
		{
			char c=*++inp;
			switch(c)
			{
				case 'n':
					rv[o++]='\n';
				break;
				case 'r':
					rv[o++]='\r';
				break;
				case '\\':
					rv[o++]='\\';
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
						if(o<=l)
							rv[o++]='0';
					}
				}
				break;
				default:
					rv[o++]='\\';
					if(o<=l)
						rv[o++]=c;
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
						buf_print(cbuf, c_status, "width set to minimum 30");
						width=30;
					}
					else
					{
						buf_print(cbuf, c_status, "width set");
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
						buf_print(cbuf, c_status, "height set to minimum 5");
						height=5;
					}
					else
					{
						buf_print(cbuf, c_status, "height set");
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
						w_buf_print(cbuf, c_status, fmsg, "");
						redraw_buffer();
					}
					else
					{
						buf_print(cbuf, c_status, "force-redraw disabled");
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
					buf_print(cbuf, c_status, "maxnicklen set");
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
					buf_print(cbuf, c_status, "mcc set");
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
					buf_print(cbuf, c_status, "buf set");
					if(force_redraw<3)
					{
						redraw_buffer();
					}
				}
				else
				{
					buf_print(cbuf, c_err, "set: No such option!");
				}
			}
			else
			{
				buf_print(cbuf, c_err, "set what?");
			}
		}
		else
		{
			buf_print(cbuf, c_err, "set what?");
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
				w_buf_print(cbuf, c_status, dstr, "");
				sprintf(cstr, "quIRC - connected to %s", server);
				settitle(cstr);
			}
		}
		else
		{
			buf_print(cbuf, c_err, "Must specify a server!");
		}
		return(0);
	}
	else if(strcmp(cmd, "disconnect")==0)
	{
		int b=bufs[cbuf].server;
		if(b>0)
		{
			buf_print(cbuf, c_status, "Disconnecting...");
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
			buf_print(cbuf, c_err, "Can't disconnect (status)!");
		}
		return(0);
	}
	else if(strcmp(cmd, "join")==0)
	{
		if(!bufs[cbuf].handle)
		{
			w_buf_print(cbuf, c_err, "/join: must be run in the context of a server!", "");
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
			buf_print(cbuf, c_err, "Must specify a channel!");
		}
		return(0);
	}
	else if((strcmp(cmd, "part")==0)||(strcmp(cmd, "leave")==0))
	{
		if(bufs[cbuf].type!=CHANNEL)
		{
			w_buf_print(cbuf, c_err, "/part: This view is not a channel!", "");
		}
		else
		{
			char partmsg[8+strlen(bufs[cbuf].bname)];
			sprintf(partmsg, "PART %s", bufs[cbuf].bname);
			irc_tx(bufs[cbuf].handle, partmsg);
			buf_print(cbuf, c_part[0], "Leaving");
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
				buf_print(cbuf, c_status, "Changing nick");
			}
			else
			{
				nick=strdup(nn);
				buf_print(cbuf, c_status, "Default nick changed");
			}
		}
		else
		{
			buf_print(cbuf, c_err, "Must specify a nickname!");
		}
		return(0);
	}
	else if(strcmp(cmd, "msg")==0)
	{
		if(!bufs[cbuf].handle)
		{
			w_buf_print(cbuf, c_err, "/msg: must be run in the context of a server!", "");
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
				char tag[maxnlen+9];
				memset(tag, ' ', maxnlen+8);
				sprintf(tag+maxnlen+2-strlen(dest), "(to %s) ", dest);
				w_buf_print(cbuf, c_msg[0], text, tag);
			}
			else
			{
				w_buf_print(cbuf, c_err, "/msg: must specify a message!", "");
			}
		}
		else
		{
				w_buf_print(cbuf, c_err, "/msg: must specify a recipient!", "");
		}
		return(0);
	}
	else if(strcmp(cmd, "me")==0)
	{
		if(bufs[cbuf].type!=CHANNEL) // TODO add PRIVATE
		{
			w_buf_print(cbuf, c_err, "/me: this view is not a channel!", "");
		}
		else if(args)
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
			w_buf_print(cbuf, c_err, "/me: must specify an action!", "");
		}
		return(0);
	}
	else if(strcmp(cmd, "cmd")==0)
	{
		if(!bufs[cbuf].handle)
		{
			w_buf_print(cbuf, c_err, "/cmd: must be run in the context of a server!", "");
		}
		else
		{
			irc_tx(bufs[cbuf].handle, args);
			w_buf_print(cbuf, c_status, args, "/cmd ");
		}
		return(0);
	}
	else
	{
		if(!cmd) cmd="";
		char dstr[30+strlen(cmd)];
		sprintf(dstr, "%s: Unrecognised command!", cmd);
		w_buf_print(cbuf, c_err, dstr, "");
		return(0);
	}
}
