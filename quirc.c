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
	#ifdef	USE_MTRACE
		mtrace();
	#endif	// USE_MTRACE
	
	sigpipe=0;
    struct sigaction sa;
    sa.sa_handler = handle_sigpipe;
    sa.sa_flags=0;
    sigemptyset(&sa.sa_mask);

    if(sigaction(SIGPIPE, &sa, NULL)==-1)
    {
    	fprintf(stderr, "Failed to set SIGPIPE handler\n");
        perror("sigaction");
        return(1);
    }

	int infc=fcntl(STDIN_FILENO, F_GETFD);
	char *fcfail=NULL;
	if(infc>=0)
	{
		if(fcntl(STDIN_FILENO, F_SETFD, infc|O_NONBLOCK)==-1)
			fcfail=strerror(errno);
	}
	if(c_init())
	{
		fprintf(stderr, "Failed to initialise colours\n"); // should be impossible
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
	
	e=initialise_buffers(buflines);
	if(e)
	{
		fprintf(stderr, "Failed to set up buffers\n");
		return(1);
	}
	
	if(fcfail)
	{
		w_buf_print(0, c_status, fcfail, "fcntl: ");
	}
	
	fd_set master, readfds;
	FD_ZERO(&master);
	FD_SET(STDIN_FILENO, &master);
	int fdmax=STDIN_FILENO;
	int servers=0;
	servlist *curr=servs;
	while(curr)
	{
		servers |= autoconnect(&master, &fdmax, curr);
		curr=curr->next;
	}
	if(!servers)
	{
		w_buf_print(0, c_status, "Not connected - use /server to connect", "");
	}
	iline inp={{NULL, 0, 0}, {NULL, 0, 0}};
	in_update(inp);
	struct timeval timeout;
	int state=0; // odd-numbered states are fatal
	while(!(state%2))
	{
		// propagate liveness values into dependent tabs, and ping idle connections
		{
			time_t now=time(NULL);
			int b;
			for(b=0;b<nbufs;b++)
			{
				if(bufs[b].live)
				{
					if(bufs[b].type==SERVER)
					{
						int idle=now-bufs[b].last;
						if(tping && (idle>tping)) // a tping value of 0 means "don't ping"
						{
							if(bufs[b].ping)
							{
								bufs[b].live=false;
								if(bufs[b].handle)
								{
									close(bufs[b].handle);
									FD_CLR(bufs[b].handle, &master);
									w_buf_print(b, c_err, "Outbound ping timeout", "Disconnected: ");
								}
								bufs[b].alert=true;
								bufs[b].hi_alert=5;
							}
							else
							{
								char pmsg[8+strlen(bufs[b].realsname)];
								sprintf(pmsg, "PING %s", bufs[b].realsname);
								irc_tx(bufs[b].handle, pmsg);
								bufs[b].ping++;
							}
							bufs[b].last=now;
							in_update(inp);
						}
					}
					else if(!bufs[bufs[b].server].live)
					{
						bufs[b].live=false;
						w_buf_print(b, c_err, "Connection to server lost", "Disconnected: ");
					}
				}
			}
		}
		timeout.tv_sec=0;
		timeout.tv_usec=250000;
		
		readfds=master;
		if(select(fdmax+1, &readfds, NULL, NULL, &timeout)==-1)
		{
			perror("\nselect");
			state=1;
		}
		else
		{
			if(timeout.tv_usec==0) // flashing of hi-alert tabs
			{
				int b;
				for(b=0;b<nbufs;b++)
				{
					switch(bufs[b].hi_alert%2)
					{
						case 1:
							bufs[b].hi_alert=bufs[b].alert?4:bufs[b].hi_alert-1;
						break;
						case 0:
							bufs[b].hi_alert=max(bufs[b].hi_alert-1, 0);
						break;
					}
				}
				in_update(inp);
			}
			int fd;
			for(fd=0;fd<=fdmax;fd++)
			{
				if(FD_ISSET(fd, &readfds))
				{
					if(fd==STDIN_FILENO)
					{
						inputchar(&inp, &state);
						/* WARNING!  Possibly non-portable code; relies on edge-case behaviour */
						bool loop=true;
						while(loop && !state)
						{
							errno=0;
							fflush(stdin);
							if(errno==ESPIPE)
							{
								inputchar(&inp, &state); // ESPIPE only happens if there is data waiting to be read
							}
							else
							{
								loop=false;
							}
						}
						break; // handle the input; everyone else can wait
					}
					else
					{
						int b;
						for(b=0;b<nbufs;b++)
						{
							if((fd==bufs[b].handle) && (bufs[b].type==SERVER))
							{
								if(bufs[b].live)
								{
									char *packet;
									int e;
									if((e=irc_rx(fd, &packet))!=0)
									{
										char emsg[64];
										sprintf(emsg, "irc_rx(%d, &%p): %d", fd, packet, e);
										close(fd);
										FD_CLR(fd, &master);
										bufs[b].live=false;
										w_buf_print(0, c_err, emsg, "error: ");
										redraw_buffer();
									}
									else if(packet)
									{
										bufs[b].ping=0;
										bufs[b].last=time(NULL);
										if(*packet)
										{
											message pkt=irc_breakdown(packet);
											if(!bufs[b].realsname && pkt.prefix)
												bufs[b].realsname=strdup(pkt.prefix);
											if(isdigit(*pkt.cmd))
											{
												irc_numeric(pkt, b);
											}
											else if(strcmp(pkt.cmd, "PING")==0)
											{
												rx_ping(pkt, b);
											}
											else if(strcmp(pkt.cmd, "PONG")==0)
											{
												// ignore, as /anything/ resets the ping-timer
											}
											else if(strcmp(pkt.cmd, "MODE")==0)
											{
												rx_mode(b);
											}
											else if(strcmp(pkt.cmd, "KILL")==0)
											{
												rx_kill(pkt, b, &master);
											}
											else if(strcmp(pkt.cmd, "KICK")==0)
											{
												rx_kick(pkt, b);
											}
											else if(strcmp(pkt.cmd, "ERROR")==0)
											{
												rx_error(pkt, b, &master);
											}
											else if(strcmp(pkt.cmd, "PRIVMSG")==0)
											{
												rx_privmsg(pkt, b, false);
											}
											else if(strcmp(pkt.cmd, "NOTICE")==0)
											{
												rx_privmsg(pkt, b, true);
											}
											else if(strcmp(pkt.cmd, "TOPIC")==0)
											{
												rx_topic(pkt, b);
											}
											else if(strcmp(pkt.cmd, "JOIN")==0)
											{
												rx_join(pkt, b);
											}
											else if(strcmp(pkt.cmd, "PART")==0)
											{
												rx_part(pkt, b);
											}
											else if(strcmp(pkt.cmd, "QUIT")==0)
											{
												rx_quit(pkt, b);
											}
											else if(strcmp(pkt.cmd, "NICK")==0)
											{
												rx_nick(pkt, b);
											}
											else
											{
												e_buf_print(b, c_unk, pkt, "Unrecognised command: ");
											}
											message_free(pkt);
										}
										else // if we called irc_breakdown, then that already free()d packet
										{
											free(packet);
										}
									}
								}
								else if(bufs[b].conninpr)
								{
									irc_conn_rest(b, nick, username, fname);
								}
								else
								{
									char emsg[32];
									sprintf(emsg, "buf %d, fd %d", b, fd);
									close(fd);
									FD_CLR(fd, &master);
									bufs[b].live=false;
									w_buf_print(0, c_err, emsg, "error: read on a dead tab: ");
									redraw_buffer();
								}
								in_update(inp);
								b=nbufs+1;
							}
						}
						if(b==nbufs)
						{
							char fmsg[48];
							sprintf(fmsg, "select() returned data on unknown fd %d!", fd);
							w_buf_print(0, c_err, fmsg, "main loop:");
							FD_CLR(fd, &master); // prevent it from happening again
						}
					}
				}
			}
		}
		switch(state)
		{
			case 3:;
				char *iptr=bufs[cbuf].input.line[(bufs[cbuf].input.ptr+bufs[cbuf].input.nlines-1)%bufs[cbuf].input.nlines];
				if(!iptr)
				{
					fprintf(stderr, "\nInternal error - state==3 and iptr is NULL!\n");
					break;
				}
				char *iinput=strdup(iptr);
				if(iinput&&*iinput) // ignore empty lines, and ignore if iinput is NULL
				{
					fflush(stdout);
					char *deq=slash_dequote(iinput); // dequote
					free(iinput);
					iinput=deq;
					if(*iinput=='/')
					{
						state=cmd_handle(iinput, &qmsg, &master, &fdmax);
						free(iinput);iinput=NULL;
					}
					else
					{
						if(bufs[cbuf].type==CHANNEL) // TODO add PRIVATE
						{
							if(bufs[cbuf].handle)
							{
								if(LIVE(cbuf))
								{
									char pmsg[12+strlen(bufs[cbuf].bname)+strlen(iinput)];
									sprintf(pmsg, "PRIVMSG %s :%s", bufs[cbuf].bname, iinput);
									irc_tx(bufs[cbuf].handle, pmsg);
									char *cnick=strdup(bufs[bufs[cbuf].server].nick);
									crush(&cnick, maxnlen);
									char tag[TAGLEN];
									mktag(tag, cnick, false);
									free(cnick);
									w_buf_print(cbuf, c_msg[0], iinput, tag);
								}
								else
								{
									w_buf_print(cbuf, c_err, "Can't talk - tab is not live!", "");
								}
							}
							else
							{
								w_buf_print(cbuf, c_err, "Can't talk - tab is disconnected!", "");
							}
						}
						else
						{
							w_buf_print(cbuf, c_err, "Can't talk - view is not a channel!", "");
						}
						free(iinput);iinput=NULL;
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
	if(state!=0)
		printf("quirc exiting\n");
	int b;
	for(b=1;b<nbufs;b++)
	{
		if((bufs[b].live) && (bufs[b].type==SERVER) && (bufs[b].handle!=0))
		{
			if(!qmsg) qmsg="quIRC Quit";
			char quit[7+strlen(qmsg)];
			sprintf(quit, "QUIT %s", qmsg);
			irc_tx(bufs[b].handle, quit);
		}
		bufs[b].live=false;
		free_buffer(b);
		b--;
	}
	bufs[0].live=false;
	free_buffer(0);
	if(bufs) free(bufs);
	if(username) free(username);
	if(fname) free(fname);
	if(nick) free(nick);
	if(portno) free(portno);
	freeservlist(servs);
	n_free(igns);
	ttyreset(STDOUT_FILENO);
	#ifdef	USE_MTRACE
		muntrace();
	#endif	// USE_MTRACE
	return(state>0?state:0);
}
