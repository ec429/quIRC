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
#include "irc.h"
#include "bits.h"
#define COLOURS	0 // activate default colours in colour.h
#include "colour.h"
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

typedef struct _name
{
	char *data;
	struct _name *next, *prev;
}
name;

int main(int argc, char *argv[])
{
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
	char *server=NULL, *portno="6667", *uname="quirc", *fname=(char *)malloc(20+strlen(VERSION_TXT)), *nick="ac", *chan=NULL;
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
			nick=argv[arg]+7;
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
	fd_set master, readfds;
	FD_ZERO(&master);
	FD_SET(STDIN_FILENO, &master);
	int fdmax=STDIN_FILENO;
	int serverhandle=0;
	if(server)
	{
		char cstr[24+strlen(server)];
		sprintf(cstr, "quIRC - connecting to %s", server);
		settitle(cstr);
		printf("Connecting to %s\n", server);
		serverhandle=irc_connect(server, portno, nick, uname, fname, &master, &fdmax);
		sprintf(cstr, "quIRC - connected to %s", server);
		settitle(cstr);
	}
	else
	{
		printf("Not connected - use /server to connect\n");
	}
	printf(CLA);
	printf("\n");
	struct timeval timeout;
	char *inp=NULL;
	char *shsrc=NULL;char *shtext=NULL;
	name *nlist=NULL;
	int state=0; // odd-numbered states are fatal
	while(!(state%2))
	{
		timeout.tv_sec=0;
		timeout.tv_usec=25000;
		if(shli && !shsrc)
		{
			shread:
			if(strncmp(shad[shlp], ">>", 2)==0)
			{
				irc_tx(serverhandle, shad[shlp]+3);
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
						char c=inp[ino]=getchar();
						inp[ino+1]=0;
						if(strchr("\010\177", c)) // various backspace-type characters
						{
							if(ino)
								inp[ino-1]=0;
							inp[ino]=0;
						}
						else if(c<32) // this also stomps on the newline 
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
									name *curr=nlist;
									name *found=NULL;bool tmany=false;
									while(curr)
									{
										if(strncmp(inp+sp, curr->data, ino-sp)==0)
										{
											if(found)
											{
												printf(CLA "\n");
												printf(LOCATE, height-2, 1);
												setcolour(c_err);
												printf(CLA "[tab] Multiple nicks match" CLR "\n" CLA "\n");
												resetcol();
												found=curr=NULL;tmany=true;
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
										printf(CLA "\n");
										printf(LOCATE, height-2, 1);
										setcolour(c_err);
										printf(CLA "[tab] No nicks match" CLR "\n" CLA "\n");
										resetcol();
									}
								}
							}
						}
						if(c=='\033') // escape sequence
						{
							if(getchar()=='\133')
							{
								switch(getchar())
								{
									case '[': // left cursor counts as a backspace
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
								}
							}
						}
						if(c=='\n')
						{
							state=3;
						}
						else
						{
							update:
							printf(LOCATE, height, 1);
							ino=inp?strlen(inp):0;
							if(ino>78)
							{
								int off=20*max((ino-53)/20, 0);
								printf("%.10s ... %s" CLR, inp, inp+off+10);
							}
							else
							{
								printf("%s" CLR, inp?inp:"");
							}	
							fflush(stdout);
						}
					}
					else if(fd==serverhandle)
					{
						char *packet;
						int e;
						if((e=irc_rx(serverhandle, &packet))!=0)
						{
							fprintf(stderr, "error: irc_rx(%d, &%p): %d\n", serverhandle, packet, e);
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
										case 353:
											// 353 dest {@|+} #chan :name [name [...]]
											strtok(NULL, " "); // dest
											strtok(NULL, " "); // @ or +, dunno what for
											char *ch=strtok(NULL, " "); // channel
											if(strcmp(ch, chan)==0)
											{
												char *nn;
												while((nn=strtok(NULL, ":@ ")))
												{
													name *new=(name *)malloc(sizeof(name));
													new->data=strdup(nn);
													new->prev=NULL;
													new->next=nlist;
													if(nlist)
														nlist->prev=new;
													nlist=new;
												}
											}
										break;
										default:
											printf(CLA "\n");
											printf(LOCATE, height-2, 1);
											setcolour(c_unn);
											printf(CLA "<<%d? %s" CLR "\n" CLA "\n", num, cmd+4);
											resetcol();
										break;
									}
								}
								else if(strcmp(cmd, "PING")==0)
								{
									char *sender=strtok(NULL, " ");
									char pong[8+strlen(uname)+strlen(sender)];
									sprintf(pong, "PONG %s %s", uname, sender+1);
									irc_tx(serverhandle, pong);
								}
								else if(strcmp(cmd, "MODE")==0)
								{
									if(chan && !join)
									{
										char joinmsg[8+strlen(chan)];
										sprintf(joinmsg, "JOIN %s", chan);
										irc_tx(serverhandle, joinmsg);
										printf(CLA "\n");
										printf(LOCATE, height-2, 1);
										setcolour(c_join[0]);
										printf(CLA "auto-Joining %s" CLR "\n" CLA "\n", chan);
										resetcol();
										join=true;
									}
									// apart from using it as a trigger, we don't look at modes just yet
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
									if(*msg==1) // CTCP
									{
										if(strncmp(msg, "\001ACTION ", 8)==0)
										{
											msg[strlen(msg)-1]=0; // remove trailing \001
											printf(CLA "\n");
											printf(LOCATE, height-2, 3+max(maxnlen-strlen(src), 0));
											setcolour(c_actn[1]);
											printf(CLA "%s ", src);
											wordline(msg+8, 3+max(maxnlen, strlen(src)));
											printf(CLR "\n" CLA "\n");
											resetcol();
										}
										else if(strncmp(msg, "\001FINGER", 7)==0)
										{
											char resp[32+strlen(src)+strlen(fname)];
											sprintf(resp, "NOTICE %s \001FINGER :%s\001", src, fname);
											irc_tx(serverhandle, resp);
										}
										else if(strncmp(msg, "\001VERSION", 8)==0)
										{
											char resp[32+strlen(src)+strlen(version)];
											sprintf(resp, "NOTICE %s \001VERSION %s:%s:%s\001", src, "quIRC", version, CC_VERSION);
											irc_tx(serverhandle, resp);
										}
									}
									else
									{
										printf(CLA "\n");
										printf(LOCATE, height-2, 1+max(maxnlen-strlen(src), 0));
										setcolour(c_msg[1]);
										printf(CLA "<%s> ", src);
										wordline(msg, 3+max(maxnlen, strlen(src)));
										printf(CLR "\n" CLA "\n");
										resetcol();
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
									printf(CLA "\n");
									printf(LOCATE, height-2, 1+max(maxnlen-strlen(src), 0));
									setcolour(c_notice[1]);
									printf(CLA "(from %s) ", src);
									wordline(msg, 9+max(maxnlen, strlen(src)));
									printf(CLR "\n" CLA "\n");
									resetcol();
								}
								else if(strcmp(cmd, "JOIN")==0)
								{
									char *dest=strtok(NULL, " \t");
									char *src=packet+1;
									char *bang=strchr(src, '!');
									if(bang)
										*bang=0;
									printf(CLA "\n");
									printf(LOCATE, height-2, 1);
									setcolour(c_join[1]);
									if(strcmp(src, nick)==0)
									{
										printf(CLA "You (%s) have joined %s" CLR "\n" CLA "\n", src, dest+1);
										chan=strdup(dest+1);
										join=true;
										char cstr[16+strlen(chan)+strlen(server)];
										sprintf(cstr, "quIRC - %s on %s", chan, server);
										settitle(cstr);
									}
									else
									{
										if(strlen(src)>maxnlen)
										{
											src[maxnlen-4]=src[maxnlen-3]=src[maxnlen-2]='.';
											src[maxnlen-1]=src[strlen(src)-1];
											src[maxnlen]=0;
										}
										printf(CLA "=%s= has joined %s" CLR "\n" CLA "\n", src, dest+1);
										name *new=(name *)malloc(sizeof(name));
										new->data=strdup(src);
										new->prev=NULL;
										new->next=nlist;
										if(nlist)
											nlist->prev=new;
										nlist=new;
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
									printf(CLA "\n");
									printf(LOCATE, height-2, 1);
									setcolour(c_part[1]);
									if(strcmp(src, nick)==0)
									{
										printf(CLA "You (%s) have left %s" CLR "\n" CLA "\n", src, dest);
										chan=NULL;
										char cstr[24+strlen(server)];
										sprintf(cstr, "quIRC - connected to %s", server);
										settitle(cstr);
										name *curr=nlist;
										while(curr)
										{
											name *next=curr->next;
											free(curr->data);
											free(curr);
											curr=next;
										}
										nlist=NULL;
									}
									else
									{
										if(strlen(src)>maxnlen)
										{
											src[maxnlen-4]=src[maxnlen-3]=src[maxnlen-2]='.';
											src[maxnlen-1]=src[strlen(src)-1];
											src[maxnlen]=0;
										}
										printf(CLA "=%s= has left %s" CLR "\n" CLA "\n", src, dest);
										name *curr=nlist;
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
													nlist=curr->next;
												}
												if(curr->next)
													curr->next->prev=curr->prev;
												free(curr->data);
												free(curr);
											}
											curr=next;
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
									printf(CLA "\n");
									printf(LOCATE, height-2, 1);
									setcolour(c_quit[1]);
									if(strcmp(src, nick)==0)
									{
										printf(CLA "You (%s) have left %s (%s)" CLR "\n" CLA "\n", src, server, dest+1); // this shouldn't happen???
									}
									else
									{
										if(strlen(src)>maxnlen)
										{
											src[maxnlen-4]=src[maxnlen-3]=src[maxnlen-2]='.';
											src[maxnlen-1]=src[strlen(src)-1];
											src[maxnlen]=0;
										}
										printf(CLA "=%s= has left %s (%s)" CLR "\n" CLA "\n", src, server, dest+1);
										name *curr=nlist;
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
													nlist=curr->next;
												}
												if(curr->next)
													curr->next->prev=curr->prev;
												free(curr->data);
												free(curr);
											}
											curr=next;
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
									printf(CLA "\n");
									printf(LOCATE, height-2, 1);
									setcolour(c_nick[1]);
									if(strcmp(dest+1, nick)==0)
									{
										printf(CLA "You (%s) are now known as %s" CLR "\n" CLA "\n", src, dest+1);
									}
									else
									{
										if(strlen(src)>maxnlen)
										{
											src[maxnlen-4]=src[maxnlen-3]=src[maxnlen-2]='.';
											src[maxnlen-1]=src[strlen(src)-1];
											src[maxnlen]=0;
										}
										printf(CLA "=%s= is now known as %s" CLR "\n" CLA "\n", src, dest+1);
										name *curr=nlist;
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
									resetcol();
								}
								else
								{
									printf(CLA "\n");
									printf(LOCATE, height-2, 1);
									setcolour(c_unk);
									printf(CLA "<? %s" CLR "\n" CLA "\n", pdata);
									resetcol();
								}
							}
							free(pdata);
							free(packet);
						}
						goto update;
					}
					else
					{
						fprintf(stderr, "\nselect() returned data on unknown fd %d!\n", fd);
						state=1;
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
						if(serverhandle)
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Already connected to a server!\n" CLA "\n");
							resetcol();
						}
						else
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
								printf(LOCATE, height-2, 1);
								printf(CLA "Connecting to %s on port %s...\n" CLA "\n", server, portno);
								serverhandle=irc_connect(server, portno, nick, uname, fname, &master, &fdmax);
							}
							else
							{
								printf(LOCATE, height-2, 1);
								setcolour(c_err);
								printf(CLA "Must specify a server!\n" CLA "\n");
								resetcol();
							}
						}
						free(inp);inp=NULL;
						state=0;
					}
					else if(strcmp(cmd, "join")==0)
					{
						if(!serverhandle)
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Not connected to a server!\n" CLA "\n");
							resetcol();
						}
						else if(chan)
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Already in a channel (quirc can currently only handle one channel)\n" CLA "\n");
							resetcol();
						}
						else if(args)
						{
							char *chan=strtok(args, " ");
							char *pass=strtok(NULL, ", ");
							if(!pass) pass="";
							char joinmsg[8+strlen(chan)+strlen(pass)];
							sprintf(joinmsg, "JOIN %s %s", chan, pass);
							irc_tx(serverhandle, joinmsg);
						}
						else
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Must specify a channel!\n" CLA "\n");
							resetcol();
						}
						free(inp);inp=NULL;
						state=0;
					}
					else if((strcmp(cmd, "part")==0)||(strcmp(cmd, "leave")==0))
					{
						if(!serverhandle)
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Not connected to a server!\n" CLA "\n");
							resetcol();
						}
						else if(!chan)
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Not in any channels!\n" CLA "\n");
							resetcol();
						}
						else if(args)
						{
							if(strcmp(chan, strtok(args, " "))==0)
							{
								char partmsg[8+strlen(chan)];
								sprintf(partmsg, "PART %s", chan);
								irc_tx(serverhandle, partmsg);
							}
							else
							{
								printf(LOCATE, height-2, 1);
								setcolour(c_err);
								printf(CLA "Not in that channel!\n" CLA "\n");
								resetcol();
							}
						}
						else
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Must specify a channel!\n" CLA "\n");
							resetcol();
						}
						free(inp);inp=NULL;
						state=0;
					}
					else if(strcmp(cmd, "nick")==0)
					{
						if(args)
						{
							char *nn=strtok(args, " ");
							nick=strdup(nn);
							if(serverhandle)
							{
								char nmsg[8+strlen(nick)];
								sprintf(nmsg, "NICK %s", nick);
								irc_tx(serverhandle, nmsg);
							}
						}
						else
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Must specify a nickname!\n" CLA "\n");
							resetcol();
						}
						free(inp);inp=NULL;
						state=0;
					}
					else if(strcmp(cmd, "msg")==0)
					{
						if(!serverhandle)
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Not connected to a server!\n" CLA "\n");
							resetcol();
						}
						else if(args)
						{
							char *dest=strtok(args, " ");
							char *text=strtok(NULL, "");
							if(text)
							{
								char privmsg[12+strlen(dest)+strlen(text)];
								sprintf(privmsg, "PRIVMSG %s %s", dest, text);
								irc_tx(serverhandle, privmsg);
								printf(LOCATE, height-2, 3+max(maxnlen-strlen(nick), 0));
								setcolour(c_msg[0]);
								printf(CLA "<to %s> ", dest);
								wordline(text, 9+max(maxnlen, strlen(dest)));
								printf(CLR "\n" CLA "\n");
								resetcol();
							}
							else
							{
								printf(LOCATE, height-2, 1);
								setcolour(c_err);
								printf(CLA "Must specify a message!\n" CLA "\n");
								resetcol();
							}
						}
						else
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Must specify a recipient!\n" CLA "\n");
							resetcol();
						}
						free(inp);inp=NULL;
						state=0;
					}
					else if(strcmp(cmd, "me")==0)
					{
						if(!serverhandle)
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Not connected to a server!\n" CLA "\n");
							resetcol();
						}
						else if(args)
						{
							char privmsg[32+strlen(chan)+strlen(args)];
							sprintf(privmsg, "PRIVMSG %s :\001ACTION %s\001", chan, args);
							irc_tx(serverhandle, privmsg);
							printf(LOCATE, height-2, 3+max(maxnlen-strlen(nick), 0));
							setcolour(c_actn[0]);
							printf(CLA "%s ", nick);
							wordline(args, 3+max(maxnlen, strlen(nick)));
							printf(CLR "\n" CLA "\n");
							resetcol();
						}
						else
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Must specify an action!\n" CLA "\n");
							resetcol();
						}
						free(inp);inp=NULL;
						state=0;
					}
					else if(strcmp(cmd, "cmd")==0)
					{
						if(!serverhandle)
						{
							printf(LOCATE, height-2, 1);
							setcolour(c_err);
							printf(CLA "Not connected to a server!\n" CLA "\n");
							resetcol();
						}
						else
						{
							irc_tx(serverhandle, args);
						}
						free(inp);inp=NULL;
						state=0;
					}
					else
					{
						printf(LOCATE, height-2, 1);
						printf(CLA "%s: Unrecognised command!\n" CLA "\n", cmd?cmd:"");
						free(inp);inp=NULL;
						state=0;
					}
				}
				else
				{
					if(serverhandle && chan)
					{
						char pmsg[12+strlen(chan)+strlen(inp)];
						sprintf(pmsg, "PRIVMSG %s :%s", chan, inp);
						irc_tx(serverhandle, pmsg);
						printf(LOCATE, height-2, 1+max(maxnlen-strlen(nick), 0));
						setcolour(c_msg[0]);
						printf(CLA "<%s> ", nick);
						wordline(inp, 3+max(maxnlen, strlen(nick)));
						printf(CLR "\n" CLA "\n");
						resetcol();
					}
					else
					{
						printf(LOCATE, height-2, 1);
						setcolour(c_err);
						printf(CLA "Can't talk - not in a channel!\n" CLA "\n");
						resetcol();
					}
					free(inp);inp=NULL;
					state=0;
				}
			break;
		}
	}
	if(state>0)
		printf("quirc exiting\n");
	if(serverhandle!=0)
	{
		if(!qmsg) qmsg="Quirc Quit.";
		char quit[7+strlen(qmsg)];
		sprintf(quit, "QUIT %s", qmsg);
		irc_tx(serverhandle, quit);
	}
	ttyreset(STDOUT_FILENO);
	return(state>0?state:0);
}
