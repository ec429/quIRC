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
#include "numeric.h"
#include "version.h"

// helper fn macros
#define max(a,b)	((a)>(b)?(a):(b))
#define min(a,b)	((a)<(b)?(a):(b))

// interface text
#define GPL_MSG "%1$s -- Copyright (C) 2010 Edward Cree\n\tThis program comes with ABSOLUTELY NO WARRANTY.\n\tThis is free software, and you are welcome to redistribute it\n\tunder certain conditions.  (GNU GPL v3+)\n\tFor further details, see the file 'COPYING' in the %1$s directory.\n", "quirc"

#define VERSION_MSG " %s %hhu.%hhu.%hhu%s\n\
 Copyright (C) 2010 Edward Cree.\n\
 License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
 This is free software: you are free to change and redistribute it.\n\
 There is NO WARRANTY, to the extent permitted by law.\n", "quirc", VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_TXT

#define USAGE_MSG "quirc [-h][-v]\n"

int main(int argc, char *argv[])
{
	int width=0, height=0;
	char *cols=getenv("COLUMNS"), *rows=getenv("ROWS");
	if(cols) sscanf(cols, "%u", &width);
	if(rows) sscanf(rows, "%u", &height);
	if(!width) width=80;
	if(!height) height=24;
	char *server=NULL, *portno="6667", *uname="quirc", *fname=(char *)malloc(20+strlen(VERSION_TXT)), *nick="ac", *chan=NULL;
	sprintf(fname, "quIRC %hhu.%hhu.%hhu%s", VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_TXT);
	char *qmsg=fname;
	char *rcfile=".quirc";
	char *rcshad=".quirc-shadow";
	bool join=false;
	FILE *rcfp=fopen(rcfile, "r");
	if(rcfp)
	{
		while(!feof(rcfp))
		{
			char *line=fgetl(rcfp);
			if(*line!='#') // #lines are comments
			{
				char *cmd=strtok(line, " \t");
				if(*cmd=='c')
				{
					// it's a colour specifier; custom colours aren't done yet
				}
				else
				{
					if(strcmp(cmd, "server")==0)
						server=strdup(strtok(NULL, "\n"));
					else if(strcmp(cmd, "nick")==0)
						nick=strdup(strtok(NULL, "\n"));
					else if(strcmp(cmd, "chan")==0)
						chan=strdup(strtok(NULL, "\n"));
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
	int maxnlen=16;
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
		else if(strcmp(argv[arg], "--no-auto-connect")==0)
		{
			server=NULL;
		}
		else if(strcmp(argv[arg], "--no-auto-join")==0)
		{
			chan=NULL;
		}
	}
	int e=ttyraw(STDOUT_FILENO);
	if(e)
	{
		fprintf(stderr, "Failed to set raw mode on tty.\n");
		perror("ttyraw");
		return(1);
	}
	settitle("quIRC - not connected");
	resetcol();
	printf(CLS);
	printf(LOCATE, height, 1);
	printf(GPL_MSG);
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
	}
	else
	{
		printf("Not connected - use /server to connect\n");
	}
	printf(CLA);
	printf("\n");
	struct timeval timeout;
	timeout.tv_sec=0;
	timeout.tv_usec=25000;
	char *inp=NULL;
	int state=0; // odd-numbered states are fatal
	while(!(state%2))
	{
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
						}
						else if(c<32) // this also stomps on the newline 
						{
							inp[ino]=0;
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
										default:
											printf(CLA "\n");
											printf(LOCATE, height-2, 1);
											setcol(1, 6, false, true);
											printf(CLA "<<%d?" CLR "\n" CLA "\n", num);
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
									if((*msg==1) && !strncmp(msg, "\001ACTION ", 8))
									{
										msg[strlen(msg)-1]=0; // remove trailing \001
										printf(CLA "\n");
										printf(LOCATE, height-2, 3+max(maxnlen-strlen(src), 0));
										setcol(0, 3, false, true);
										printf(CLA "%s %s" CLR "\n" CLA "\n", src, msg+8);
										resetcol();
									}
									else
									{
										printf(CLA "\n");
										printf(LOCATE, height-2, 1+max(maxnlen-strlen(src), 0));
										printf(CLA "<%s> %s" CLR "\n" CLA "\n", src, msg);
									}
								}
								else if(strcmp(cmd, "NOTICE")==0)
								{
									char *msg=cmd+strlen(cmd)+1; // prefixed with :
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
									printf(CLA "\n");
									printf(LOCATE, height-2, 1+max(maxnlen-strlen(src), 0));
									setcol(7, 0, true, false);
									printf(CLA "<%s> %s" CLR "\n" CLA "\n", src, msg);
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
									setcol(2, 0, true, false);
									if(strcmp(src, nick)==0)
									{
										printf(CLA "You (%s) have joined %s" CLR "\n" CLA "\n", src, dest+1);
										chan=strdup(dest+1);
										join=true;
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
									setcol(2, 0, true, false);
									if(strcmp(src, nick)==0)
									{
										printf(CLA "You (%s) have left %s" CLR "\n" CLA "\n", src, dest);
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
									setcol(3, 0, true, false);
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
									setcol(4, 0, true, false);
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
									}
									resetcol();
								}
								else
								{
									printf(CLA "\n");
									printf(LOCATE, height-2, 1);
									setcol(3, 4, false, false);
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
							setcol(1, 0, true, false);
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
								setcol(1, 0, true, false);
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
							setcol(1, 0, true, false);
							printf(CLA "Not connected to a server!\n" CLA "\n");
							resetcol();
						}
						else if(chan)
						{
							printf(LOCATE, height-2, 1);
							setcol(1, 0, true, false);
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
							setcol(1, 0, true, false);
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
							setcol(1, 0, true, false);
							printf(CLA "Not connected to a server!\n" CLA "\n");
							resetcol();
						}
						else if(!chan)
						{
							printf(LOCATE, height-2, 1);
							setcol(1, 0, true, false);
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
								setcol(1, 0, true, false);
								printf(CLA "Not in that channel!\n" CLA "\n");
								resetcol();
							}
						}
						else
						{
							printf(LOCATE, height-2, 1);
							setcol(1, 0, true, false);
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
							setcol(1, 0, true, false);
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
							setcol(1, 0, true, false);
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
							}
							else
							{
								printf(LOCATE, height-2, 1);
								setcol(1, 0, true, false);
								printf(CLA "Must specify a message!\n" CLA "\n");
								resetcol();
							}
						}
						else
						{
							printf(LOCATE, height-2, 1);
							setcol(1, 0, true, false);
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
							setcol(1, 0, true, false);
							printf(CLA "Not connected to a server!\n" CLA "\n");
							resetcol();
						}
						else if(args)
						{
							char privmsg[32+strlen(chan)+strlen(args)];
							sprintf(privmsg, "PRIVMSG %s :\001ACTION %s\001", chan, args);
							irc_tx(serverhandle, privmsg);
							printf(LOCATE, height-2, 3+max(maxnlen-strlen(nick), 0));
							setcol(0, 3, false, true);
							printf(CLA "%s %s" CLR "\n" CLA "\n", nick, args);
							resetcol();
						}
						else
						{
							printf(LOCATE, height-2, 1);
							setcol(1, 0, true, false);
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
							setcol(1, 0, true, false);
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
						setcol(7, 0, false, true);
						printf(CLA "<%s> %s" CLR "\n" CLA "\n", nick, inp);
						resetcol();
					}
					else
					{
						printf(LOCATE, height-2, 1);
						setcol(1, 0, true, false);
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
