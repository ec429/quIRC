/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "quirc.h"

int main(int argc, char *argv[])
{
	if(c_init())
	{
		fprintf(stderr, "Failed to initialise colours (malloc failure)\n");
		return(1);
	}
	if(def_config())
	{
		fprintf(stderr, "Failed to apply default configuration\n");
		return(1);
	}
	settitle("quIRC - not connected");
	resetcol();
	char *qmsg=fname;
	char *rcfile=".quirc";
	char *home=getenv("HOME");
	if(home) chdir(home);
	bool join=false;
	FILE *rcfp=fopen(rcfile, "r");
	if(rcfp)
	{
		rcread(rcfp);
		fclose(rcfp);
	}
	
	signed int e=pargs(argc, argv);
	if(e>=0)
	{
		return(e);
	}
	
	e=ttyraw(STDOUT_FILENO);
	if(e)
	{
		fprintf(stderr, "Failed to set raw mode on tty\n");
		perror("ttyraw");
		return(1);
	}
	
	int i;
	for(i=0;i<height;i++) // push old stuff off the top of the screen, so it's preserved
		printf("\n");
	
	e=initialise_buffers(buflines, nick);
	if(e)
	{
		fprintf(stderr, "Failed to set up buffers\n");
		return(1);
	}
	
	fd_set master, readfds;
	FD_ZERO(&master);
	FD_SET(STDIN_FILENO, &master);
	int fdmax=STDIN_FILENO;
	int serverhandle=0;
	if(server)
	{
		serverhandle = autoconnect(&master, &fdmax);
	}
	if(!serverhandle)
	{
		w_buf_print(0, c_status, "Not connected - use /server to connect", "");
	}
	in_update("");
	struct timeval timeout;
	char *inp=NULL;
	int state=0; // odd-numbered states are fatal
	while(!(state%2))
	{
		timeout.tv_sec=0;
		timeout.tv_usec=250000;
		
		fflush(stdin);
		readfds=master;
		if(select(fdmax+1, &readfds, NULL, NULL, &timeout)==-1)
		{
			perror("\nselect");
			state=1;
		}
		else
		{
			int fd;
			for(fd=0;fd<=fdmax;fd++)
			{
				if(FD_ISSET(fd, &readfds))
				{
					if(fd==0)
					{
						inputchar(&inp, &state);
					}
					else
					{
						int b;
						for(b=0;b<nbufs;b++)
						{
							if((fd==bufs[b].handle) && (bufs[b].type==SERVER))
							{
								char *packet;
								int e;
								if((e=irc_rx(fd, &packet))!=0)
								{
									char emsg[64];
									sprintf(emsg, "error: irc_rx(%d, &%p): %d", fd, packet, e);
									cbuf=0;
									redraw_buffer();
									w_buf_print(0, c_err, emsg, "");
									state=5;
									qmsg="client crashed";
								}
								else if(packet)
								{
									char *pdata=strdup(packet);
									if(packet[0])
									{
										char *p=packet;
										if(*p==':')
										{
											p=strchr(p, ' ');
										}
										char *cmd=strtok(p, " ");
										if(*packet==':') *p=0;
										if(isdigit(*cmd))
										{
											irc_numeric(cmd, b);
										}
										else if(strcmp(cmd, "PING")==0)
										{
											rx_ping(fd);
										}
										else if(strcmp(cmd, "MODE")==0)
										{
											rx_mode(&join, b);
										}
										else if(strcmp(cmd, "KILL")==0)
										{
											rx_kill(b, &master);
										}
										else if(strcmp(cmd, "ERROR")==0)
										{
											rx_error(b, &master);
										}
										else if(strcmp(cmd, "PRIVMSG")==0)
										{
											rx_privmsg(b, packet, pdata);
										}
										else if(strcmp(cmd, "NOTICE")==0)
										{
											rx_notice(b, packet);
										}
										else if(strcmp(cmd, "TOPIC")==0)
										{
											rx_topic(b, packet);
										}
										else if(strcmp(cmd, "JOIN")==0)
										{
											rx_join(b, packet, pdata, &join);
										}
										else if(strcmp(cmd, "PART")==0)
										{
											rx_part(b, packet, pdata);
										}
										else if(strcmp(cmd, "QUIT")==0)
										{
											rx_quit(b, packet, pdata);
										}
										else if(strcmp(cmd, "NICK")==0)
										{
											rx_nick(b, packet, pdata);
										}
										else
										{
											w_buf_print(b, c_unk, pdata, "<? ");
										}
									}
									free(pdata);
									free(packet);
								}
								in_update(inp);
								b=nbufs+1;
							}
						}
						if(b==nbufs)
						{
							fprintf(stderr, "\nselect() returned data on unknown fd %d!\n", fd);
							state=1;
						}
					}
				}
			}
		}
		switch(state)
		{
			case 3:
				if(!inp)
				{
					fprintf(stderr, "\nInternal error - state==3 and inp is NULL!\n");
					break;
				}
				if(*inp)
				{
					printf(SCROLLDOWN);
					fflush(stdout);
					char *deq=slash_dequote(inp); // dequote
					free(inp);
					inp=deq;
					if(*inp=='/')
					{
						state=cmd_handle(inp, &qmsg, &master, &fdmax);
						free(inp);inp=NULL;
					}
					else
					{
						if(bufs[cbuf].type==CHANNEL) // TODO add PRIVATE
						{
							char pmsg[12+strlen(bufs[cbuf].bname)+strlen(inp)];
							sprintf(pmsg, "PRIVMSG %s :%s", bufs[cbuf].bname, inp);
							irc_tx(bufs[cbuf].handle, pmsg);
							while(inp[strlen(inp)-1]=='\n')
								inp[strlen(inp)-1]=0; // stomp out trailing newlines, they break things
							char tag[maxnlen+4];
							memset(tag, ' ', maxnlen+3);
							char *cnick=strdup(bufs[bufs[cbuf].server].nick);
							crush(&cnick, maxnlen);
							sprintf(tag+maxnlen-strlen(cnick), "<%s> ", cnick);
							free(cnick);
							w_buf_print(cbuf, c_msg[0], inp, tag);
						}
						else
						{
							w_buf_print(cbuf, c_err, "Can't talk - view is not a channel!", "");
						}
						free(inp);inp=NULL;
						state=0;
					}
				}
				else
				{
					state=0;
				}
				if(force_redraw==2) // 'slight paranoia' mode
				{
					redraw_buffer();
				}
				if(force_redraw<3)
				{
					in_update(inp);
				}
			break;
		}
		if(force_redraw>=3) // 'extremely paranoid' mode
		{
			redraw_buffer();
			in_update(inp);
		}
	}
	if(state>0)
		printf("quirc exiting\n");
	int b;
	for(b=0;b<nbufs;b++)
	{
		if(bufs[b].handle!=0)
		{
			if(!qmsg) qmsg="Quirc Quit.";
			char quit[7+strlen(qmsg)];
			sprintf(quit, "QUIT %s", qmsg);
			irc_tx(bufs[b].handle, quit);
		}
	}
	ttyreset(STDOUT_FILENO);
	return(state>0?state:0);
}
