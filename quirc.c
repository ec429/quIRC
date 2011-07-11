/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-11 Edward Cree

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
	
	bufs=NULL;
	
	init_start_buffer();
	
	sigpipe=0;
	struct sigaction sa;
	sa.sa_handler=handle_sigpipe;
	sa.sa_flags=0;
	sigemptyset(&sa.sa_mask);
	
	if(sigaction(SIGPIPE, &sa, NULL)==-1)
	{
		fprintf(stderr, "Failed to set SIGPIPE handler\n");
		perror("sigaction");
		push_buffer();
		return(1);
	}
	if(sigaction(SIGWINCH, &sa, NULL)==-1)
	{
		fprintf(stderr, "Failed to set SIGWINCH handler\n");
		perror("sigaction");
		push_buffer();
		return(1);
	}
	
	int infc=fcntl(STDIN_FILENO, F_GETFD);
	if(infc>=0)
	{
		if(fcntl(STDIN_FILENO, F_SETFD, infc|O_NONBLOCK)==-1)
		{
			char *err=strerror(errno);
			char msg[48+strlen(err)];
			sprintf(msg, "Failed to mark stdin non-blocking: fcntl: %s", err);
			asb_failsafe(c_status, msg);
		}
	}
	if(initkeys())
	{
		fprintf(stderr, "Failed to initialise keymapping\n");
		push_buffer();
		return(1);
	}
	if(c_init()) // should be impossible
	{
		fprintf(stderr, "Failed to initialise colours\n");
		push_buffer();
		return(1);
	}
	if(def_config())
	{
		fprintf(stderr, "Failed to apply default configuration\n");
		push_buffer();
		return(1);
	}
	resetcol();
	char *qmsg=fname;
	char *home=getenv("HOME");
	if(home)
	{
		char *qfld=malloc(strlen(home)+8);
		if(qfld)
		{
			sprintf(qfld, "%s/.quirc", home);
			if(chdir(qfld))
			{
				fprintf(stderr, "Failed to change directory into %s: %s", qfld, strerror(errno));
				push_buffer();
				return(1);
			}
			free(qfld);
		}
		else
		{
			perror("Failed to allocate space for 'qfld'");
			push_buffer();
			return(1);
		}
	}
	else
	{
		fprintf(stderr, "Environment variable $HOME not set!  Exiting\n");
		return(1);
	}
	FILE *rcfp=fopen("rc", "r");
	int rc_err=0;
	if(rcfp)
	{
		rc_err=rcread(rcfp);
		fclose(rcfp);
		if(rc_err)
		{
			char msg[32];
			sprintf(msg, "%d errors in ~/.quirc/rc", rc_err);
			asb_failsafe(c_status, msg);
		}
	}
	else
	{
		asb_failsafe(c_status, "no config file found.  Install one at ~/.quirc/rc");
	}
	FILE *keyfp=fopen("keys", "r");
	if(keyfp)
	{
		loadkeys(keyfp);
		fclose(keyfp);
	}
	signed int e=pargs(argc, argv);
	if(e>=0)
	{
		push_buffer();
		return(e);
	}
	
	conf_check();
	
	e=ttyraw(STDOUT_FILENO);
	if(e)
	{
		fprintf(stderr, "Failed to set raw mode on tty\n");
		perror("ttyraw");
		push_buffer();
		return(1);
	}
	
	unsigned int i;
	for(i=0;i<height;i++) // push old stuff off the top of the screen, so it's preserved
		printf("\n");
	
	e=initialise_buffers(buflines);
	if(e)
	{
		fprintf(stderr, "Failed to set up buffers\n");
		push_buffer();
		return(1);
	}
	
	push_buffer();
	
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
	if((!servers)&&(!quiet))
	{
		add_to_buffer(0, c_status, "Not connected - use /server to connect", "");
	}
	iline inp={{NULL, 0, 0}, {NULL, 0, 0}};
	in_update(inp);
	struct timeval timeout;
	int state=0; // odd-numbered states are fatal
	while(!(state%2))
	{
		if(sigwinch)
		{
			if(winch)
			{
				int l, c;
				if(termsize(STDIN_FILENO, &c, &l))
				{
					add_to_buffer(0, c_err, strerror(errno), "termsize: ioctl: ");
				}
				else
				{
					height=max(l, 5);
					width=max(c, 30);
				}
			}
			sigwinch=0;
		}
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
						unsigned int idle=now-bufs[b].last;
						if(tping && (idle>tping)) // a tping value of 0 means "don't ping"
						{
							if(bufs[b].ping)
							{
								bufs[b].live=false;
								if(bufs[b].handle)
								{
									close(bufs[b].handle);
									FD_CLR(bufs[b].handle, &master);
									bufs[b].handle=0; // de-bind fd
									add_to_buffer(b, c_err, "Outbound ping timeout", "Disconnected: ");
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
						bufs[b].handle=0; // just in case
						add_to_buffer(b, c_err, "Connection to server lost", "Disconnected: ");
					}
				}
			}
		}
		timeout.tv_sec=0;
		timeout.tv_usec=250000;
		
		readfds=master;
		if(select(fdmax+1, &readfds, NULL, NULL, &timeout)==-1)
		{
			add_to_buffer(0, c_err, strerror(errno), "select: ");
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
										bufs[b].handle=0; // de-bind fd
										FD_CLR(fd, &master);
										bufs[b].live=false;
										add_to_buffer(0, c_err, emsg, "error: ");
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
									bufs[b].handle=0; // de-bind fd
									FD_CLR(fd, &master);
									bufs[b].live=false;
									add_to_buffer(0, c_err, emsg, "error: read on a dead tab: ");
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
							if(!quiet) add_to_buffer(0, c_err, fmsg, "main loop:");
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
					}
					else
					{
						talk(iinput);
						state=0;
					}
					free(iinput);iinput=NULL;
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
