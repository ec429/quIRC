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
	char *rcshad=".quirc-shadow";
	char *home=getenv("HOME");
	if(home) chdir(home);
	bool join=false;
	FILE *rcfp=fopen(rcfile, "r");
	if(rcfp)
	{
		rcread(rcfp);
		fclose(rcfp);
	}
	FILE *rcsfp=fopen(rcshad, "r"); // this is the shadow file, which will be replaced by proper qu-script later
	int shli=0;
	char **shad=NULL;
	if(rcsfp)
	{
		while(!feof(rcsfp))
		{
			shad=(char **)realloc(shad, ++shli*sizeof(char *));
			shad[shli-1]=fgetl(rcsfp);
		}
		fclose(rcsfp);
	}
	int shlp=0;
	
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
		buf_print(0, c_status, "Not connected - use /server to connect", true);
	}
	in_update("");
	struct timeval timeout;
	char *inp=NULL;
	char *shsrc=NULL;char *shtext=NULL;
	int state=0; // odd-numbered states are fatal
	while(!(state%2))
	{
		timeout.tv_sec=0;
		timeout.tv_usec=250000;
		if(shli && !shsrc) // TODO: proper scripting capability with regex-match on the << (expectation) lines and attachment to a buffer
		{
			shread:
			if(strncmp(shad[shlp], ">>", 2)==0)
			{
				irc_tx(bufs[1].handle, shad[shlp]+3); // because of how auto-ident works, this should always be on buffer 1 (the first server)
			}
			if(strncmp(shad[shlp], "<<", 2)==0) // read
			{
				shsrc=strtok(shad[shlp]+3, " \n");
				shtext=strtok(NULL, " \n");
				if(strcmp(shtext, "*")==0)
					shtext=NULL;
			}
			else
			{
				shlp++;
				if(shlp>=shli) // end reached, wipe it out
				{
					for(shlp=0;shlp<shli;shlp++)
						free(shad[shlp]);
					shli=0;
				}
				else
					goto shread;
			}
		}
		
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
							if(fd==bufs[b].handle)
							{
								char *packet;
								int e;
								if((e=irc_rx(fd, &packet))!=0)
								{
									char emsg[64];
									sprintf(emsg, "error: irc_rx(%d, &%p): %d", fd, packet, e);
									cbuf=0;
									redraw_buffer();
									buf_print(0, c_err, emsg, true);
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
											rx_mode(fd, &join, b);
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
											char *dest=strtok(NULL, " ");
											char *msg=dest+strlen(dest)+2; // prefixed with :
											char *src=packet+1;
											char *bang=strchr(src, '!');
											if(bang)
												*bang=0;
											if(shli)
											{
												if(strcmp(src, shsrc)==0)
												{
													if((!shtext)||strcmp(msg, shtext))
													{
														shlp++;
														shsrc=NULL;
														if(shlp>=shli) // end reached, wipe it out
														{
															for(shlp=0;shlp<shli;shlp++)
																free(shad[shlp]);
															shli=0;
														}
													}
												}
											}
											if(strlen(src)>maxnlen)
											{
												src[maxnlen-4]=src[maxnlen-3]=src[maxnlen-2]='.';
												src[maxnlen-1]=src[strlen(src)-1];
												src[maxnlen]=0;
											}
											char *out=(char *)malloc(16+max(maxnlen, strlen(src)));
											memset(out, ' ', max(maxnlen-strlen(src), 0));
											out[max(maxnlen-strlen(src), 0)]=0;
											sprintf(out+strlen(out), "(from %s) ", src);
											wordline(msg, 9+max(maxnlen, strlen(src)), &out);
											buf_print(b, c_notice[1], out, true);
											free(out);
										}
										else if(strcmp(cmd, "JOIN")==0)
										{
											char *dest=strtok(NULL, " \t");
											char *src=packet+1;
											char *bang=strchr(src, '!');
											if(bang)
												*bang=0;
											if(strcmp(src, bufs[b].nick)==0)
											{
												char dstr[20+strlen(src)+strlen(dest+1)];
												sprintf(dstr, "You (%s) have joined %s", src, dest+1);
												chan=strdup(dest+1);
												join=true;
												char cstr[16+strlen(src)+strlen(bufs[b].bname)];
												sprintf(cstr, "quIRC - %s on %s", src, bufs[b].bname);
												settitle(cstr);
												bufs=(buffer *)realloc(bufs, ++nbufs*sizeof(buffer));
												init_buffer(nbufs-1, CHANNEL, chan, buflines);
												bufs[nbufs-1].server=bufs[b].server;
												cbuf=nbufs-1;
												buf_print(cbuf, c_join[1], dstr, true);
												bufs[cbuf].handle=bufs[bufs[cbuf].server].handle;
											}
											else
											{
												int b2;
												bool match=false;
												for(b2=0;b2<nbufs;b2++)
												{
													if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcasecmp(dest+1, bufs[b2].bname)==0))
													{
														match=true;
														char dstr[16+strlen(src)+strlen(dest+1)];
														sprintf(dstr, "=%s= has joined %s", src, dest+1);
														buf_print(b2, c_join[1], dstr, true);
														n_add(&bufs[b2].nlist, src);
													}
												}
												if(!match)
												{
													char dstr[4+strlen(pdata)];
													sprintf(dstr, "?? %s", pdata);
													buf_print(b, c_err, dstr, true);
												}
											}
										}
										else if(strcmp(cmd, "PART")==0)
										{
											char *dest=strtok(NULL, " \t");
											char *src=packet+1;
											char *bang=strchr(src, '!');
											if(bang)
												*bang=0;
											if(strcmp(src, bufs[b].nick)==0)
											{
												chan=NULL;
												int b2;
												for(b2=0;b2<nbufs;b2++)
												{
													if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcasecmp(dest, bufs[b2].bname)==0))
													{
														if(b2==cbuf)
														{
															cbuf=b;
															char cstr[24+strlen(bufs[b].bname)];
															sprintf(cstr, "quIRC - connected to %s", bufs[b].bname);
															settitle(cstr);
														}
														free_buffer(b2);
													}
												}
											}
											else
											{
												int b2;
												bool match=false;
												for(b2=0;b2<nbufs;b2++)
												{
													if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcasecmp(dest, bufs[b2].bname)==0))
													{
														match=true;
														char dstr[16+strlen(src)+strlen(dest)];
														sprintf(dstr, "=%s= has left %s", src, dest);
														buf_print(b2, c_part[1], dstr, true);
														n_cull(&bufs[b2].nlist, src);
													}
												}
												if(!match)
												{
													char dstr[4+strlen(pdata)];
													sprintf(dstr, "?? %s", pdata);
													buf_print(b, c_err, dstr, true);
												}
											}
										}
										else if(strcmp(cmd, "QUIT")==0)
										{
											char *dest=strtok(NULL, "");
											char *src=packet+1;
											char *bang=strchr(src, '!');
											if(bang)
												*bang=0;
											setcolour(c_quit[1]);
											if(strcmp(src, bufs[b].nick)==0) // this shouldn't happen???
											{
												int b2;
												for(b2=0;b2<nbufs;b2++)
												{
													if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL))
													{
														char dstr[24+strlen(src)+strlen(bufs[b].bname)+strlen(dest+1)];
														sprintf(dstr, "You (%s) have left %s (%s)", src, bufs[b].bname, dest+1);
														buf_print(b2, c_quit[1], dstr, true);
													}
												}
											}
											else
											{
												int b2;
												for(b2=0;b2<nbufs;b2++)
												{
													if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL))
													{
														if(n_cull(&bufs[b2].nlist, src))
														{
															char dstr[24+strlen(src)+strlen(bufs[b].bname)+strlen(dest+1)];
															sprintf(dstr, "=%s= has left %s (%s)", src, bufs[b].bname, dest+1);
															buf_print(b2, c_quit[1], dstr, true);
														}
													}
												}
											}
										}
										else if(strcmp(cmd, "NICK")==0)
										{
											char *dest=strtok(NULL, " \t");
											char *src=packet+1;
											char *bang=strchr(src, '!');
											if(bang)
												*bang=0;
											setcolour(c_nick[1]);
											if(strcmp(dest+1, bufs[b].nick)==0)
											{
												char dstr[30+strlen(src)+strlen(dest+1)];
												sprintf(dstr, "You (%s) are now known as %s", src, dest+1);
												int b2;
												for(b2=0;b2<nbufs;b2++)
												{
													if(bufs[b2].server==b)
													{
														buf_print(b2, c_nick[1], dstr, true);
														n_cull(&bufs[b2].nlist, src);
														n_add(&bufs[b2].nlist, dest+1);
													}
												}
											}
											else
											{
												int b2;
												bool match=false;
												for(b2=0;b2<nbufs;b2++)
												{
													if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL))
													{
														match=true;
														if(n_cull(&bufs[b2].nlist, src))
														{
															n_add(&bufs[b2].nlist, dest+1);
															char dstr[30+strlen(src)+strlen(dest+1)];
															sprintf(dstr, "=%s= is now known as %s", src, dest+1);
															buf_print(b2, c_nick[1], dstr, true);
														}
													}
												}
												if(!match)
												{
													char dstr[4+strlen(pdata)];
													sprintf(dstr, "?? %s", pdata);
													buf_print(b, c_err, dstr, true);
												}
											}
										}
										else
										{
											char dstr[5+strlen(pdata)];
											sprintf(dstr, "<? %s", pdata);
											buf_print(b, c_unk, dstr, true);
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
				if(*inp=='/')
				{
					char *cmd=inp+1;
					char *args=strchr(cmd, ' ');
					if(args) *args++=0;
					if((strcmp(cmd, "quit")==0)||(strcmp(cmd, "exit")==0))
					{
						if(args) qmsg=args;
						printf(LOCATE, height-2, 1);
						printf(CLA "Exited quirc\n" CLA "\n");
						free(inp);inp=NULL;
						state=-1;
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
									buf_print(cbuf, c_status, "width set", false);
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
									buf_print(cbuf, c_status, "height set", false);
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
										buf_print(cbuf, c_status, fmsg, false);
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
						free(inp);inp=NULL;
						state=0;
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
							int serverhandle=irc_connect(server, portno, nick, username, fname, &master, &fdmax);
							if(serverhandle)
							{
								bufs=(buffer *)realloc(bufs, ++nbufs*sizeof(buffer));
								init_buffer(nbufs-1, SERVER, server, buflines);
								cbuf=nbufs-1;
								bufs[cbuf].handle=serverhandle;
								bufs[cbuf].nick=strdup(nick);
								bufs[cbuf].server=cbuf;
								add_to_buffer(cbuf, c_status, dstr);
								sprintf(cstr, "quIRC - connected to %s", server);
								settitle(cstr);
							}
						}
						else
						{
							buf_print(cbuf, c_err, "Must specify a server!", false);
						}
						free(inp);inp=NULL;
						state=0;
					}
					else if(strcmp(cmd, "disconnect")==0)
					{
						int b=bufs[cbuf].server;
						if(b>0)
						{
							buf_print(cbuf, c_status, "Disconnecting...", false);
							close(bufs[b].handle);
							FD_CLR(bufs[b].handle, &master);
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
						free(inp);inp=NULL;
						state=0;
					}
					else if(strcmp(cmd, "join")==0)
					{
						if(!bufs[cbuf].handle)
						{
							buf_print(cbuf, c_err, "/join: must be run in the context of a server!", false);
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
						free(inp);inp=NULL;
						state=0;
					}
					else if((strcmp(cmd, "part")==0)||(strcmp(cmd, "leave")==0))
					{
						if(bufs[cbuf].type!=CHANNEL)
						{
							buf_print(cbuf, c_err, "/part: This view is not a channel!", false);
						}
						else
						{
							char partmsg[8+strlen(bufs[cbuf].bname)];
							sprintf(partmsg, "PART %s", bufs[cbuf].bname);
							irc_tx(bufs[cbuf].handle, partmsg);
							buf_print(cbuf, c_part[0], "Leaving", false);
						}
						free(inp);inp=NULL;
						state=0;
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
						free(inp);inp=NULL;
						state=0;
					}
					else if(strcmp(cmd, "msg")==0)
					{
						if(!bufs[cbuf].handle)
						{
							buf_print(cbuf, c_err, "/msg: must be run in the context of a server!", false);
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
								buf_print(cbuf, c_err, "/msg: must specify a message!", false);
							}
						}
						else
						{
								buf_print(cbuf, c_err, "/msg: must specify a recipient!", false);
						}
						free(inp);inp=NULL;
						state=0;
					}
					else if(strcmp(cmd, "me")==0)
					{
						if(bufs[cbuf].type!=CHANNEL) // TODO add PRIVATE
						{
							buf_print(cbuf, c_err, "/me: this view is not a channel!", false);
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
							buf_print(cbuf, c_err, "/me: must specify an action!", false);
						}
						free(inp);inp=NULL;
						state=0;
					}
					else if(strcmp(cmd, "cmd")==0)
					{
						if(!bufs[cbuf].handle)
						{
							buf_print(cbuf, c_err, "/cmd: must be run in the context of a server!", false);
						}
						else
						{
							irc_tx(bufs[cbuf].handle, args);
						}
						free(inp);inp=NULL;
						state=0;
					}
					else
					{
						if(!cmd) cmd="";
						char dstr[30+strlen(cmd)];
						sprintf(dstr, "%s: Unrecognised command!", cmd);
						buf_print(cbuf, c_err, dstr, false);
						free(inp);inp=NULL;
						state=0;
					}
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
						char *out=(char *)malloc(3+max(maxnlen, strlen(bufs[bufs[cbuf].server].nick)));
						memset(out, ' ', max(maxnlen-strlen(bufs[bufs[cbuf].server].nick), 0));
						out[max(maxnlen-strlen(bufs[bufs[cbuf].server].nick), 0)]=0;
						sprintf(out+strlen(out), "<%s> ", bufs[bufs[cbuf].server].nick);
						wordline(inp, 3+max(maxnlen, strlen(bufs[bufs[cbuf].server].nick)), &out);
						buf_print(cbuf, c_msg[0], out, false);
						free(out);
					}
					else
					{
						buf_print(cbuf, c_err, "Can't talk - view is not a channel!", false);
					}
					free(inp);inp=NULL;
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
