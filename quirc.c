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

#define _GNU_SOURCE // feature test macro

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>

#include "ttyraw.h"
#include "ttyesc.h"
#define COLOURS	0 // activate default colours in colour.h
#include "colour.h"
#include "irc.h"
#include "bits.h"
#include "buffer.h"
#include "numeric.h"
#include "version.h"

// helper fn macros
#define max(a,b)	((a)>(b)?(a):(b))
#define min(a,b)	((a)<(b)?(a):(b))

// interface text
#define GPL_MSG "%1$s -- Copyright (C) 2010 Edward Cree\n\tThis program comes with ABSOLUTELY NO WARRANTY.\n\tThis is free software, and you are welcome to redistribute it\n\tunder certain conditions.  (GNU GPL v3+)\n\tFor further details, see the file 'COPYING' in the %1$s directory.\n", "quirc"

#define VERSION_MSG " %s %hhu.%hhu.%hhu%s%s\n\
 Copyright (C) 2010 Edward Cree.\n\
 License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
 This is free software: you are free to change and redistribute it.\n\
 There is NO WARRANTY, to the extent permitted by law.\n", "quirc", VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_TXT[0]?"-":"", VERSION_TXT

#define USAGE_MSG "quirc [-h][-v]\n"

int main(int argc, char *argv[])
{
	int buflines=256; // will make configable later
	mirc_colour_compat=1; // silently strip
	char *cols=getenv("COLUMNS"), *rows=getenv("ROWS");
	if(cols) sscanf(cols, "%u", &width);
	if(rows) sscanf(rows, "%u", &height);
	if(!width) width=80;
	if(!height) height=24;
	settitle("quIRC - not connected");
	resetcol();
	printf(LOCATE, height, 1);
	printf("\n");
	int maxnlen=16;
	char *server=NULL, *portno="6667", *uname="quirc", *fname=(char *)malloc(20+strlen(VERSION_TXT)), *nick=strdup("ac"), *chan=NULL;
	sprintf(fname, "quIRC %hhu.%hhu.%hhu%s%s", VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_TXT[0]?"-":"", VERSION_TXT);
	char version[16+strlen(VERSION_TXT)];
	sprintf(version, "%hhu.%hhu.%hhu%s%s", VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_TXT[0]?"-":"", VERSION_TXT);
	char *qmsg=fname;
	char *rcfile=".quirc";
	char *rcshad=".quirc-shadow";
	char *home=getenv("HOME");
	if(home) chdir(home);
	bool join=false;
	FILE *rcfp=fopen(rcfile, "r");
	if(rcfp)
	{
		while(!feof(rcfp))
		{
			char *line=fgetl(rcfp);
			if(!strchr("#\n", *line)) // #lines are comments
			{
				char *cmd=strtok(line, " \t");
				if(*cmd=='%')
				{
					int which=0;
					// custom colours
					if(*++cmd=='S')
					{
						which=1;
						cmd++;
					}
					else if(*cmd=='R')
					{
						which=2;
						cmd++;
					}
					colour *what=NULL;
					if(strcmp(cmd, "msg")==0)
					{
						what=c_msg;
					}
					else if(strcmp(cmd, "notice")==0)
					{
						what=c_notice;
					}
					else if(strcmp(cmd, "join")==0)
					{
						what=c_join;
					}
					else if(strcmp(cmd, "part")==0)
					{
						what=c_part;
					}
					else if(strcmp(cmd, "quit")==0)
					{
						what=c_quit;
					}
					else if(strcmp(cmd, "nick")==0)
					{
						what=c_nick;
					}
					else if(strcmp(cmd, "act")==0)
					{
						what=c_actn;
					}
					else if(strcmp(cmd, "status")==0)
					{
						what=&c_status;which=-1;
					}
					else if(strcmp(cmd, "err")==0)
					{
						what=&c_err;which=-1;
					}
					else if(strcmp(cmd, "unk")==0)
					{
						what=&c_unk;which=-1;
					}
					else if(strcmp(cmd, "unn")==0)
					{
						what=&c_unn;which=-1;
					}
					if(what)
					{
						char *spec=strtok(NULL, "\n");
						colour new;int hi, ul;
						sscanf(spec, "%d %d %d %d", &new.fore, &new.back, &hi, &ul);
						new.hi=hi;new.ul=ul;
						if(which!=2)
							what[0]=new;
						if((which%2)==0)
							what[1]=new;
					}
				}
				else
				{
					char *rest=strtok(NULL, "\n");
					if(strcmp(cmd, "server")==0)
						server=strdup(rest);
					else if(strcmp(cmd, "port")==0)
						portno=strdup(rest);
					else if(strcmp(cmd, "uname")==0)
						uname=strdup(rest);
					else if(strcmp(cmd, "fname")==0)
						fname=strdup(rest);
					else if(strcmp(cmd, "nick")==0)
						nick=strdup(rest);
					else if(strcmp(cmd, "chan")==0)
						chan=strdup(rest);
					else if(strcmp(cmd, "mnln")==0)
						sscanf(rest, "%u", &maxnlen);
					else if(strcmp(cmd, "mcc")==0)
						sscanf(rest, "%u", &mirc_colour_compat);
					else
					{
						fprintf(stderr, "Unrecognised cmd %s in .quirc (ignoring)\n", cmd);
					}
				}
			}
			free(line);
		}
		fclose(rcfp);
	}
	FILE *rcsfp=fopen(rcshad, "r");
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
	int arg;
	for(arg=1;arg<argc;arg++)
	{
		if((strcmp(argv[arg], "--help")==0)||(strcmp(argv[arg], "-h")==0))
		{
			fprintf(stderr, USAGE_MSG);
			return(0);
		}
		else if((strcmp(argv[arg], "--version")==0)||(strcmp(argv[arg], "-v")==0))
		{
			fprintf(stderr, VERSION_MSG);
			return(0);
		}
		else if(strncmp(argv[arg], "--width=", 8)==0) // just in case you need to force them
		{
			sscanf(argv[arg]+8, "%u", &width);
		}
		else if(strncmp(argv[arg], "--height=", 9)==0)
		{
			sscanf(argv[arg]+9, "%u", &height);
		}
		else if(strncmp(argv[arg], "--maxnicklen=", 13)==0)
		{
			sscanf(argv[arg]+13, "%u", &maxnlen);
		}
		else if(strncmp(argv[arg], "--mcc=", 6)==0)
		{
			sscanf(argv[arg]+6, "%u", &mirc_colour_compat);
		}
		else if(strcmp(argv[arg], "--no-auto-connect")==0)
		{
			server=NULL;
		}
		else if(strcmp(argv[arg], "--no-auto-join")==0)
		{
			chan=NULL;
		}
		else if(strncmp(argv[arg], "--server=", 9)==0)
			server=argv[arg]+9;
		else if(strncmp(argv[arg], "--port=", 7)==0)
			portno=argv[arg]+7;
		else if(strncmp(argv[arg], "--uname=", 8)==0)
			uname=argv[arg]+8;
		else if(strncmp(argv[arg], "--fname=", 8)==0)
			fname=argv[arg]+8;
		else if(strncmp(argv[arg], "--nick=", 7)==0)
			nick=strdup(argv[arg]+7);
		else if(strncmp(argv[arg], "--chan=", 7)==0)
			chan=argv[arg]+7;
	}
	printf(GPL_MSG);
	int e=ttyraw(STDOUT_FILENO);
	if(e)
	{
		fprintf(stderr, "Failed to set raw mode on tty.\n");
		perror("ttyraw");
		return(1);
	}
	bufs=(buffer *)malloc(sizeof(buffer));
	init_buffer(0, STATUS, "status", buflines); // buf 0 is always STATUS
	nbufs=1;
	cbuf=0;
	bufs[0].nick=strdup(nick);
	fd_set master, readfds;
	FD_ZERO(&master);
	FD_SET(STDIN_FILENO, &master);
	int fdmax=STDIN_FILENO;
	printf(CLA);
	printf("\n");
	if(server)
	{
		char cstr[36+strlen(server)+strlen(portno)];
		sprintf(cstr, "quIRC - connecting to %s", server);
		settitle(cstr);
		sprintf(cstr, "Connecting to %s on port %s...", server, portno);
		setcolour(c_status);
		printf(CLA "\n");
		printf(LOCATE, height-2, 1);
		printf("%s" CLR "\n", cstr);
		resetcol();
		printf(CLA "\n");
		int serverhandle=irc_connect(server, portno, nick, uname, fname, &master, &fdmax);
		if(serverhandle)
		{
			bufs=(buffer *)realloc(bufs, ++nbufs*sizeof(buffer));
			init_buffer(1, SERVER, server, buflines);
			cbuf=1;
			bufs[cbuf].handle=serverhandle;
			bufs[cbuf].nick=strdup(nick);
			bufs[cbuf].server=1;
			add_to_buffer(1, c_status, cstr);
			sprintf(cstr, "quIRC - connected to %s", server);
			settitle(cstr);
		}
	}
	else
	{
		buf_print(0, c_status, "Not connected - use /server to connect", true);
	}
	struct timeval timeout;
	char *inp=NULL;
	char *shsrc=NULL;char *shtext=NULL;
	int state=0; // odd-numbered states are fatal
	while(!(state%2))
	{
		timeout.tv_sec=0;
		timeout.tv_usec=25000;
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
						printf("\010\010\010" CLA);
						int ino=inp?strlen(inp):0;
						inp=(char *)realloc(inp, ino+2);
						unsigned char c=inp[ino]=getchar();
						inp[ino+1]=0;
						if(strchr("\010\177", c)) // various backspace-type characters
						{
							if(ino)
								inp[ino-1]=0;
							inp[ino]=0;
						}
						else if((c<32)||(c>127)) // this also stomps on the newline 
						{
							inp[ino]=0;
							if(c==1)
							{
								free(inp);
								inp=NULL;
								ino=0;
							}
							if(ino>0)
							{
								if(c=='\t') // tab completion of nicks
								{
									int sp=ino-1;
									while(sp>0 && !strchr(" \t", inp[sp-1]))
										sp--;
									name *curr=bufs[cbuf].nlist;
									name *found=NULL;bool tmany=false;
									while(curr)
									{
										if(strncasecmp(inp+sp, curr->data, ino-sp)==0)
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
										inp=(char *)realloc(inp, sp+strlen(found->data)+4);
										if(sp)
											sprintf(inp+sp, "%s", found->data);
										else
											sprintf(inp+sp, "%s: ", found->data);
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
								switch(getchar())
								{
									case 'D': // left cursor counts as a backspace
										if(ino)
											inp[ino-1]=0;
									break;
									case '3': // take another
										if(getchar()=='~') // delete
										{
											if(ino)
												inp[ino-1]=0;
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
												}
												
											}
										}
									break;
								}
							}
						}
						else if(c==0xc2) // c2 bN = alt-N (for N in 0...9)
						{
							char d=getchar();
							if((d&0xf0)==0xb0)
							{
								cbuf=min(max(d&0x0f, 0), nbufs-1);
								redraw_buffer();
							}
						}
						if(c=='\n')
						{
							state=3;
						}
						else
						{
							in_update(inp);
						}
					}
					else
					{
						int b;
						for(b=0;b<nbufs;b++)
						{
							if(fd==bufs[b].handle)
							{
								char *packet; // TODO detect appropriate destination buffer for packet
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
											int num=0;
											sscanf(cmd, "%d", &num);
											switch(num)
											{
												case RPL_NAMREPLY:
													// 353 dest {@|+} #chan :name [name [...]]
													strtok(NULL, " "); // dest
													strtok(NULL, " "); // @ or +, dunno what for
													char *ch=strtok(NULL, " "); // channel
													int b2;
													for(b2=0;b2<nbufs;b2++)
													{
														if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcmp(ch, bufs[b2].bname)==0))
														{
															char *nn;
															while((nn=strtok(NULL, ":@ ")))
															{
																name *new=(name *)malloc(sizeof(name));
																new->data=strdup(nn);
																new->prev=NULL;
																new->next=bufs[b2].nlist;
																if(bufs[b2].nlist)
																	bufs[b2].nlist->prev=new;
																bufs[b2].nlist=new;
															}
														}
													}
												break;
												case RPL_MOTDSTART:
												case RPL_MOTD:
												case RPL_ENDOFMOTD:
													// silently ignore the motd, because they're always far too long and annoying
												break;
												default:
													;
													char umsg[16+strlen(cmd+4)];
													sprintf(umsg, "<<%d? %s", num, cmd+4);
													buf_print(b, c_unn, umsg, true);
												break;
											}
										}
										else if(strcmp(cmd, "PING")==0)
										{
											char *sender=strtok(NULL, " ");
											char pong[8+strlen(uname)+strlen(sender)];
											sprintf(pong, "PONG %s %s", uname, sender+1);
											irc_tx(fd, pong);
										}
										else if(strcmp(cmd, "MODE")==0)
										{
											if(chan && !join)
											{
												char joinmsg[8+strlen(chan)];
												sprintf(joinmsg, "JOIN %s", chan);
												irc_tx(fd, joinmsg);
												char jmsg[16+strlen(chan)];
												sprintf(jmsg, "auto-Joining %s", chan);
												buf_print(b, c_join[0], jmsg, true);
												join=true;
											}
											// apart from using it as a trigger, we don't look at modes just yet
										}
										else if(strcmp(cmd, "KILL")==0)
										{
											char *dest=strtok(NULL, " \t"); // user to be killed
											if(strcmp(dest, bufs[b].nick)==0) // if it's us
											{
												close(fd);
												FD_CLR(fd, &master);
												int b2;
												for(b2=1;b2<nbufs;b2++)
												{
													while((b2<nbufs) && ((bufs[b2].server==b) || (bufs[b2].server==0)))
													{
														free_buffer(b2);
														if(b2==cbuf)
															cbuf=0;
													}
												}
												redraw_buffer();
											}
										}
										else if(strcmp(cmd, "ERROR")==0) // assume it's fatal
										{
											close(fd);
											FD_CLR(fd, &master);
											int b2;
											for(b2=1;b2<nbufs;b2++)
											{
												while((b2<nbufs) && ((bufs[b2].server==b) || (bufs[b2].server==0)))
												{
													free_buffer(b2);
													if(b2==cbuf)
														cbuf=0;
												}
											}
											redraw_buffer();
										}
										else if(strcmp(cmd, "PRIVMSG")==0)
										{
											char *dest=strtok(NULL, " \t");
											char *msg=dest+strlen(dest)+2; // prefixed with :
											char *src=packet+1;
											char *bang=strchr(src, '!');
											if(bang)
												*bang=0;
											if(strlen(src)>maxnlen)
											{
												src[maxnlen-4]=src[maxnlen-3]=src[maxnlen-2]='.';
												src[maxnlen-1]=src[strlen(src)-1];
												src[maxnlen]=0;
											}
											int b2;
											for(b2=0;b2<nbufs;b2++)
											{
												if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcmp(dest, bufs[b2].bname)==0))
												{
													if(*msg==1) // CTCP
													{
														if(strncmp(msg, "\001ACTION ", 8)==0)
														{
															msg[strlen(msg)-1]=0; // remove trailing \001
															char *out=(char *)malloc(5+max(maxnlen, strlen(src)));
															memset(out, ' ', 2+max(maxnlen-strlen(src), 0));
															out[2+max(maxnlen-strlen(src), 0)]=0;
															strcat(out, src);
															strcat(out, " ");
															wordline(msg+8, 3+max(maxnlen, strlen(src)), &out);
															buf_print(b2, c_actn[1], out, true);
															free(out);
														}
														else if(strncmp(msg, "\001FINGER", 7)==0)
														{
															char resp[32+strlen(src)+strlen(fname)];
															sprintf(resp, "NOTICE %s \001FINGER :%s\001", src, fname);
															irc_tx(fd, resp);
														}
														else if(strncmp(msg, "\001VERSION", 8)==0)
														{
															char resp[32+strlen(src)+strlen(version)];
															sprintf(resp, "NOTICE %s \001VERSION %s:%s:%s\001", src, "quIRC", version, CC_VERSION);
															irc_tx(fd, resp);
														}
													}
													else
													{
														char *out=(char *)malloc(5+max(maxnlen, strlen(src)));
														memset(out, ' ', max(maxnlen-strlen(src), 0));
														out[max(maxnlen-strlen(src), 0)]=0;
														sprintf(out+strlen(out), "<%s> ", src);
														wordline(msg, 3+max(maxnlen, strlen(src)), &out);
														buf_print(b2, c_msg[1], out, true);
														free(out);
													}
												}
											}
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
												if(strlen(src)>maxnlen)
												{
													src[maxnlen-4]=src[maxnlen-3]=src[maxnlen-2]='.';
													src[maxnlen-1]=src[strlen(src)-1];
													src[maxnlen]=0;
												}
												int b2;
												bool match=false;
												for(b2=0;b2<nbufs;b2++)
												{
													if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcmp(dest+1, bufs[b2].bname)==0))
													{
														match=true;
														char dstr[16+strlen(src)+strlen(dest+1)];
														sprintf(dstr, "=%s= has joined %s", src, dest+1);
														buf_print(b2, c_join[1], dstr, true);
														name *curr=bufs[b2].nlist; // cull existing copies of this nick
														while(curr)
														{
															name *next=curr->next;
															if(strcmp(curr->data, src)==0)
															{
																if(curr->prev)
																{
																	curr->prev->next=curr->next;
																}
																else
																{
																	bufs[b2].nlist=curr->next;
																}
																if(curr->next)
																	curr->next->prev=curr->prev;
																free(curr->data);
																free(curr);
															}
															curr=next;
														}
														name *new=(name *)malloc(sizeof(name));
														new->data=strdup(src);
														new->prev=NULL;
														new->next=bufs[b2].nlist;
														if(bufs[b2].nlist)
															bufs[b2].nlist->prev=new;
														bufs[b2].nlist=new;
													}
												}
												if(!match)
												{
													char dstr[4+strlen(pdata)];
													sprintf(dstr, "?? %s", pdata);
													buf_print(b, c_err, dstr, true);
												}
											}
											resetcol();
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
													if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcmp(dest, bufs[b2].bname)==0))
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
												if(strlen(src)>maxnlen)
												{
													src[maxnlen-4]=src[maxnlen-3]=src[maxnlen-2]='.';
													src[maxnlen-1]=src[strlen(src)-1];
													src[maxnlen]=0;
												}
												int b2;
												bool match=false;
												for(b2=0;b2<nbufs;b2++)
												{
													if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcmp(dest, bufs[b2].bname)==0))
													{
														match=true;
														char dstr[16+strlen(src)+strlen(dest)];
														sprintf(dstr, "=%s= has left %s", src, dest);
														buf_print(b2, c_part[1], dstr, true);
														name *curr=bufs[b2].nlist;
														while(curr)
														{
															name *next=curr->next;
															if(strcmp(curr->data, src)==0)
															{
																if(curr->prev)
																{
																	curr->prev->next=curr->next;
																}
																else
																{
																	bufs[b2].nlist=curr->next;
																}
																if(curr->next)
																	curr->next->prev=curr->prev;
																free(curr->data);
																free(curr);
															}
															curr=next;
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
											resetcol();
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
												if(strlen(src)>maxnlen)
												{
													src[maxnlen-4]=src[maxnlen-3]=src[maxnlen-2]='.';
													src[maxnlen-1]=src[strlen(src)-1];
													src[maxnlen]=0;
												}
												int b2;
												for(b2=0;b2<nbufs;b2++)
												{
													if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL))
													{
														name *curr=bufs[b2].nlist;
														while(curr)
														{
															name *next=curr->next;
															if(strcmp(curr->data, src)==0)
															{
																char dstr[24+strlen(src)+strlen(bufs[b].bname)+strlen(dest+1)];
																sprintf(dstr, "=%s= has left %s (%s)", src, bufs[b].bname, dest+1);
																buf_print(b2, c_quit[1], dstr, true);
																if(curr->prev)
																{
																	curr->prev->next=curr->next;
																}
																else
																{
																	bufs[b2].nlist=curr->next;
																}
																if(curr->next)
																	curr->next->prev=curr->prev;
																free(curr->data);
																free(curr);
															}
															curr=next;
														}
													}
												}
											}
											resetcol();
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
														name *curr=bufs[b2].nlist;
														while(curr)
														{
															if(strcmp(curr->data, src)==0)
															{
																free(curr->data);
																curr->data=strdup(dest+1);
															}
															curr=curr->next;
														}
													}
												}
											}
											else
											{
												if(strlen(src)>maxnlen)
												{
													src[maxnlen-4]=src[maxnlen-3]=src[maxnlen-2]='.';
													src[maxnlen-1]=src[strlen(src)-1];
													src[maxnlen]=0;
												}
												int b2;
												bool match=false;
												for(b2=0;b2<nbufs;b2++)
												{
													if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL))
													{
														match=true;
														name *curr=bufs[b2].nlist;
														while(curr)
														{
															if(strcmp(curr->data, src)==0)
															{
																free(curr->data);
																curr->data=strdup(dest+1);
																char dstr[30+strlen(src)+strlen(dest+1)];
																sprintf(dstr, "=%s= is now known as %s", src, dest+1);
																buf_print(b2, c_nick[1], dstr, true);
															}
															curr=curr->next;
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
											resetcol();
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
							int serverhandle=irc_connect(server, portno, nick, uname, fname, &master, &fdmax);
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
							redraw_buffer();
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
								sprintf(privmsg, "PRIVMSG %s %s", dest, text);
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
				in_update(inp);
			break;
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
