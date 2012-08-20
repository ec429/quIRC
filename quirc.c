/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

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
	
	setupterm((char *)0, fileno(stdout), (int *)0);
	putp(keypad_xmit);
	init_ring(&s_buf);
	
	sigpipe=0;
	sigwinch=0;
	sigusr1=0;
	struct sigaction sa;
	sa.sa_handler=handle_signals;
	sa.sa_flags=0;
	sigemptyset(&sa.sa_mask);
	
	if(sigaction(SIGPIPE, &sa, NULL)==-1)
	{
		fprintf(stderr, "Failed to set SIGPIPE handler\n");
		perror("sigaction");
		push_ring(&s_buf, QUIET);
		termsgr0();
		return(1);
	}
	if(sigaction(SIGWINCH, &sa, NULL)==-1)
	{
		fprintf(stderr, "Failed to set SIGWINCH handler\n");
		perror("sigaction");
		push_ring(&s_buf, QUIET);
		termsgr0();
		return(1);
	}
	if(sigaction(SIGUSR1, &sa, NULL)==-1)
	{
		fprintf(stderr, "Failed to set SIGUSR1 handler\n");
		perror("sigaction");
		push_ring(&s_buf, QUIET);
		termsgr0();
		return(1);
	}
	
	int infc=fcntl(fileno(stdin), F_GETFD);
	if(infc>=0)
	{
		if(fcntl(fileno(stdin), F_SETFD, infc|O_NONBLOCK)==-1)
		{
			char *err=strerror(errno);
			char msg[48+strlen(err)];
			sprintf(msg, "Failed to mark stdin non-blocking: fcntl: %s", err);
			atr_failsafe(&s_buf, STA, msg, "init: ");
		}
	}
	if(initkeys())
	{
		fprintf(stderr, "Failed to initialise keymapping\n");
		push_ring(&s_buf, QUIET);
		termsgr0();
		return(1);
	}
	if(c_init()) // should be impossible
	{
		fprintf(stderr, "Failed to initialise colours\n");
		push_ring(&s_buf, QUIET);
		termsgr0();
		return(1);
	}
	if(def_config())
	{
		fprintf(stderr, "Failed to apply default configuration\n");
		push_ring(&s_buf, QUIET);
		termsgr0();
		return(1);
	}
	resetcol();
	char *qmsg=strdup(fname);
	char *home=getenv("HOME");
	bool haveqfld=true;
	if(home)
	{
		char *qfld=malloc(strlen(home)+8);
		if(qfld)
		{
			sprintf(qfld, "%s/.quirc", home);
			if(chdir(qfld))
			{
				char *err=strerror(errno);
				char msg[48+strlen(err)+strlen(qfld)];
				sprintf(msg, "Failed to change directory into %s: chdir: %s", qfld, err);
				atr_failsafe(&s_buf, STA, msg, "init: ");
				haveqfld=false;
			}
			free(qfld);
		}
		else
		{
			perror("Failed to allocate space for 'qfld'");
			push_ring(&s_buf, QUIET);
			termsgr0();
			return(1);
		}
	}
	else
	{
		fprintf(stderr, "Environment variable $HOME not set!  Exiting\n");
		push_ring(&s_buf, QUIET);
		termsgr0();
		return(1);
	}
	FILE *rcfp=haveqfld?fopen("rc", "r"):NULL;
	int rc_err=0;
	if(rcfp)
	{
		rc_err=rcread(rcfp);
		fclose(rcfp);
		if(rc_err)
		{
			char msg[32];
			sprintf(msg, "%d errors in ~/.quirc/rc", rc_err);
			atr_failsafe(&s_buf, STA, msg, "init: ");
		}
	}
	else
	{
		atr_failsafe(&s_buf, STA, "no config file found.  Install one at ~/.quirc/rc", "init: ");
	}
	FILE *keyfp=fopen("keys", "r");
	if(haveqfld&&keyfp)
	{
		loadkeys(keyfp);
		fclose(keyfp);
	}
	signed int e=pargs(argc, argv);
	if(e>=0)
	{
		push_ring(&s_buf, QUIET);
		termsgr0();
		return(e);
	}
	
	conf_check();
	
	if(ttyraw(fileno(stdout)))
	{
		atr_failsafe(&s_buf, ERR, "Failed to set raw mode on tty: ttyraw:", "init: ");
		atr_failsafe(&s_buf, ERR, strerror(errno), "init: ");
	}
	
	unsigned int i;
	for(i=0;i<height;i++) // push old stuff off the top of the screen, so it's preserved
		printf("\n");
	
	e=initialise_buffers(buflines);
	if(e)
	{
		fprintf(stderr, "Failed to set up buffers\n");
		push_ring(&s_buf, QUIET);
		termsgr0();
		return(1);
	}
	
	push_ring(&s_buf, QUIET);
	
	fd_set master, readfds;
	FD_ZERO(&master);
	FD_SET(fileno(stdin), &master);
	int fdmax=fileno(stdin);
	if(!autoconnect(&master, &fdmax, servs))
		add_to_buffer(0, STA, QUIET, 0, false, "Not connected - use /server to connect", "");
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
				if(termsize(fileno(stdin), &c, &l))
				{
					add_to_buffer(0, ERR, NORMAL, 0, false, strerror(errno), "termsize: ioctl: ");
				}
				else
				{
					height=max(l, 5);
					width=max(c, 30);
					for(int buf=0;buf<nbufs;buf++)
						bufs[buf].dirty=true;
					redraw_buffer();
				}
			}
			sigwinch=0;
		}
		#if ASYNCH_NL
		if(sigusr1)
		{
			sigusr1=0; // no race condition, because after this point we loop over the whole nl_list
			int serverhandle;
			nl_list *list=nl_active;
			while((list=nl_active)&&(serverhandle=irc_conn_found(&list, &master, &fdmax))&&list)
			{
				const char *server=list->nl_details->ar_name;
				if(!server) server="server";
				const char *port=list->nl_details->ar_service;
				if(!port) port="port";
				char dstr[32+strlen(server)+strlen(port)];
				sprintf(dstr, "Found %s, connecting on %s...", server, port);
				if(force_redraw<3) redraw_buffer();
				if(list->reconn_b)
				{
					bufs[list->reconn_b].handle=serverhandle;
					int b2;
					for(b2=1;b2<nbufs;b2++)
					{
						if(bufs[b2].server==list->reconn_b)
							bufs[b2].handle=serverhandle;
					}
					bufs[list->reconn_b].conninpr=true;
					free(bufs[list->reconn_b].realsname);
					bufs[list->reconn_b].realsname=NULL;
					if(list->autoent)
						bufs[list->reconn_b].autoent=list->autoent;
					add_to_buffer(list->reconn_b, STA, QUIET, 0, false, dstr, "/server: ");
				}
				else
				{
					bufs=(buffer *)realloc(bufs, ++nbufs*sizeof(buffer));
					init_buffer(nbufs-1, SERVER, server, buflines);
					cbuf=nbufs-1;
					bufs[cbuf].handle=serverhandle;
					bufs[cbuf].nick=strdup(nick);
					bufs[cbuf].server=cbuf;
					bufs[cbuf].conninpr=true;
					if(list->autoent)
					{
						free(bufs[cbuf].nick);
						bufs[cbuf].nick=strdup(list->autoent->nick);
						bufs[cbuf].ilist=n_dup(list->autoent->igns);
						bufs[cbuf].autoent=list->autoent;
					}
					add_to_buffer(cbuf, STA, QUIET, 0, false, dstr, "/server: ");
				}
				free((char *)list->nl_details->ar_name);
				free((char *)list->nl_details->ar_service);
				freeaddrinfo((void *)list->nl_details->ar_request);
				if(list->prev) list->prev->next=list->next;
				else nl_active=list->next;
				if(list->next) list->next->prev=list->prev;
				free(list);
			}
		}
		#endif
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
									add_to_buffer(b, ERR, NORMAL, 0, false, "Outbound ping timeout", "Disconnected: ");
								}
								bufs[b].alert=true;
								bufs[b].hi_alert=5;
							}
							else
							{
								char pmsg[8+strlen(bufs[b].realsname?bufs[b].realsname:bufs[b].bname)];
								sprintf(pmsg, "PING %s", bufs[b].realsname?bufs[b].realsname:bufs[b].bname);
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
						add_to_buffer(b, ERR, NORMAL, 0, false, "Connection to server lost", "Disconnected: ");
					}
				}
			}
		}
		timeout.tv_sec=0;
		timeout.tv_usec=250000;
		
		readfds=master;
		if(select(fdmax+1, &readfds, NULL, NULL, &timeout)==-1)
		{
			if(errno!=EINTR) // nobody cares if select() was interrupted by a signal
				add_to_buffer(0, ERR, NORMAL, 0, false, strerror(errno), "select: ");
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
					if(fd==fileno(stdin))
					{
						inputchar(&inp, &state);
						/* XXX Possibly non-portable code; relies on edge-case behaviour */
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
									if((e=irc_rx(fd, &packet, &master))!=0)
									{
										char emsg[64];
										sprintf(emsg, "irc_rx(%d, &%p): %d", fd, packet, e);
										close(fd);
										bufs[b].handle=0; // de-bind fd
										FD_CLR(fd, &master);
										bufs[b].live=false;
										add_to_buffer(0, ERR, NORMAL, 0, false, emsg, "error: ");
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
												rx_mode(pkt, b);
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
												e_buf_print(b, UNK, pkt, "Unrecognised command: ");
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
									irc_conn_rest(b, nick, username, pass, fname);
								}
								else
								{
									char emsg[32];
									sprintf(emsg, "buf %d, fd %d", b, fd);
									close(fd);
									bufs[b].handle=0; // de-bind fd
									FD_CLR(fd, &master);
									bufs[b].live=false;
									add_to_buffer(0, ERR, NORMAL, 0, false, emsg, "error: read on a dead tab: ");
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
							add_to_buffer(0, ERR, QUIET, 0, false, fmsg, "main loop:");
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
	free(bufs);
	free(username);
	free(fname);
	free(qmsg);
	free(nick);
	free(portno);
	freeservlist(servs);
	n_free(igns);
	locate(height-1, 0);
	putchar('\n');
	ttyreset(fileno(stdout));
	termsgr0();
	#ifdef	USE_MTRACE
		muntrace();
	#endif	// USE_MTRACE
	return(state>0?state:0);
}
