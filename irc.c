/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	irc: networking functions
*/

#include "irc.h"

int irc_connect(char *server, char *portno, char *nick, char *username, char *fullname, fd_set *master, int *fdmax)
{
	int serverhandle;
	struct addrinfo hints, *servinfo;
	// Look up server
	memset(&hints, 0, sizeof(hints));
	hints.ai_family=AF_INET;
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	int rv;
	if((rv = getaddrinfo(server, portno, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n\n", gai_strerror(rv));
		return(0); // 0 indicates failure as rv is new serverhandle value
	}
	char sip[INET_ADDRSTRLEN];
	struct addrinfo *p;
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		inet_ntop(p->ai_family, &(((struct sockaddr_in*)p->ai_addr)->sin_addr), sip, sizeof(sip));
		if((serverhandle = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("socket");
			fprintf(stderr, "\n");
			continue;
		}
		if(connect(serverhandle, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(serverhandle);
			perror("connect");
			fprintf(stderr, "\n");
			continue;
		}
		break;
	}
	if (p == NULL)
	{
		fprintf(stderr, "failed to connect\n\n");
		return(0); // 0 indicates failure as rv is new serverhandle value
	}
	freeaddrinfo(servinfo); // don't need this any more
	
	FD_SET(serverhandle, master);
	*fdmax=max(*fdmax, serverhandle);
	
	char nickmsg[6+strlen(nick)];
	sprintf(nickmsg, "NICK %s", nick);
	irc_tx(serverhandle, nickmsg);
	struct utsname arch;
	uname(&arch);
	char usermsg[12+strlen(username)+strlen(arch.nodename)+strlen(fullname)];
	sprintf(usermsg, "USER %s %s %s :%s", username, arch.nodename, arch.nodename, fullname);
	irc_tx(serverhandle, usermsg);
	return(serverhandle);
}

int autoconnect(fd_set *master, int *fdmax)
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
	int serverhandle=irc_connect(server, portno, nick, username, fname, master, fdmax);
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
	return(serverhandle);
}

int irc_tx(int fd, char * packet)
{
	//printf(">> %s\n\n", packet); // for debugging purposes
	unsigned long l=strlen(packet)+1;
	unsigned long p=0;
	while(p<l)
	{
		signed long j=send(fd, packet+p, l-p, 0);
		if(j<1)
			return(p); // Something went wrong with send()!
		p+=j;
	}
	send(fd, "\n", 1, 0);
	return(l); // Return the number of bytes sent
}

int irc_rx(int fd, char ** data)
{
	*data=(char *)malloc(512);
	if(!*data)
		return(1);
	int l=0;
	bool cr=false;
	while(!cr)
	{
		long bytes=recv(fd, (*data)+l, 1, MSG_WAITALL);
		if(bytes>0)
		{
			char c=(*data)[l];
			if((strchr("\n\r", c)!=NULL) || (l>510))
			{
				cr=true;
				(*data)[l]=0;
			}
			l++;
		}
	}
	return(0);
}

int irc_numeric(char *cmd, int b) // TODO check the strtok()s for NULLs
{
	int num=0;
	char *ch;
	int b2;
	sscanf(cmd, "%d", &num);
	char *dest=strtok(NULL, " "); // TODO: check it's for us
	int skip=0;
	switch(num)
	{
		case RPL_NAMREPLY:
			// 353 dest {@|+} #chan :name [name [...]]
			strtok(NULL, " "); // @ or +, dunno what for
			ch=strtok(NULL, " "); // channel
			for(b2=0;b2<nbufs;b2++)
			{
				if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcasecmp(ch, bufs[b2].bname)==0))
				{
					if(!bufs[b2].namreply)
					{
						bufs[b2].namreply=true;
						n_free(bufs[b2].nlist);
						bufs[b2].nlist=NULL;
					}
					char *nn;
					while((nn=strtok(NULL, ":@ ")))
					{
						n_add(&bufs[b2].nlist, nn);
					}
				}
			}
		break;
		case RPL_ENDOFNAMES:
			// 366 dest #chan :End of /NAMES list
			ch=strtok(NULL, " "); // channel
			for(b2=0;b2<nbufs;b2++)
			{
				if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcasecmp(ch, bufs[b2].bname)==0))
				{
					bufs[b2].namreply=false;
					char lmsg[32+strlen(ch)];
					sprintf(lmsg, "NAMES list received for %s", ch);
					buf_print(b2, c_status, lmsg, true);
				}
			}
		break;
		case RPL_ENDOFMOTD: // 376 dest :End of MOTD command
			/* fallthrough */
		case RPL_MOTDSTART: // 375 dest :- <server> Message of the day -
			skip=1;
			/* fallthrough */
		case RPL_MOTD: // 372 dest :- <text>
			if(!skip) skip=3;
			char *motdline=strtok(NULL, "");
			if(strlen(motdline)>=skip) motdline+=skip;
			buf_print(b, c_notice[1], motdline, true);
		break;
		case RPL_TOPIC: // 332 dest <channel> :<topic>
			ch=strtok(NULL, " "); // channel
			char *topic=strtok(NULL, "")+1;
			for(b2=0;b2<nbufs;b2++)
			{
				if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcasecmp(ch, bufs[b2].bname)==0))
				{
					char tmsg[32+strlen(ch)+strlen(topic)];
					sprintf(tmsg, "Topic for %s is %s", ch, topic);
					buf_print(b2, c_notice[1], tmsg, true);
				}
			}
		break;
		case RPL_NOTOPIC: // 331 dest <channel> :No topic is set
			ch=strtok(NULL, " "); // channel
			for(b2=0;b2<nbufs;b2++)
			{
				if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcasecmp(ch, bufs[b2].bname)==0))
				{
					char tmsg[32+strlen(ch)];
					sprintf(tmsg, "No topic is set for %s", ch);
					buf_print(b2, c_notice[1], tmsg, true);
				}
			}
		break;
		case RPL_X_TOPICWASSET: // 331 dest <channel> <nick> <time>
			ch=strtok(NULL, " "); // channel
			char *nick=strtok(NULL, " "); // by whom?
			char *time=strtok(NULL, ""); // when?
			for(b2=0;b2<nbufs;b2++)
			{
				if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcasecmp(ch, bufs[b2].bname)==0))
				{
					time_t when;
					sscanf(time, "%u", (unsigned int *)&when);
					char ts[256];
					struct tm *tm = gmtime(&when);
					size_t tslen = strftime(ts, sizeof(ts), "%H:%M:%S GMT on %a, %d %b %Y", tm); // TODO options controlling date format (eg. ISO 8601)
					char tmsg[32+strlen(nick)+tslen];
					sprintf(tmsg, "Topic was set by %s at %s", nick, ts);
					buf_print(b2, c_status, tmsg, true);
				}
			}
		break;
		default:
			;
			char *rest=strtok(NULL, "");
			char umsg[16+strlen(dest)+strlen(rest)];
			sprintf(umsg, "<<%d? %s %s", num, dest, rest);
			buf_print(b, c_unn, umsg, true);
		break;
	}
	return(num);
}
