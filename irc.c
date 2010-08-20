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
	char *rest;
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
		case RPL_MOTDSTART: // 375 dest :- <server> Message of the day -
			skip=1;
			/* fallthrough */
		case RPL_MOTD: // 372 dest :- <text>
			if(!skip) skip=3;
			char *motdline=strtok(NULL, "");
			if(strlen(motdline)>=skip) motdline+=skip;
			buf_print(b, c_notice[1], motdline, true);
		break;
		case ERR_NOMOTD: // 422 <dest> :MOTD File is missing
			rest=strtok(NULL, "");
			buf_print(b, c_notice[1], rest+1, true);
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
		case RPL_LUSERCLIENT: // 251 <dest> :There are <integer> users and <integer> invisible on <integer> servers"
		case RPL_LUSERME: // 255 <dest> ":I have <integer> clients and <integer> servers
			rest=strtok(NULL, "");
			buf_print(b, c_status, rest+1, true);
		break;
		case RPL_LUSEROP: // 252 <dest> <integer> :operator(s) online
		case RPL_LUSERUNKNOWN: // 253 <dest> "<integer> :unknown connection(s)
		case RPL_LUSERCHANNELS: // 254 <dest> "<integer> :channels formed
			{
				char *count=strtok(NULL, " ");
				rest=strtok(NULL, "");
				char lmsg[2+strlen(count)+strlen(rest)];
				sprintf(lmsg, "%s %s", count, rest+1);
				buf_print(b, c_status, lmsg, true);
			}
		break;
		case RPL_X_LOCALUSERS: // 265 <dest> :Current Local Users: <integer>\tMax: <integer>
		case RPL_X_GLOBALUSERS: // 266 <dest> :Current Global Users: <integer>\tMax: <integer>
			rest=strtok(NULL, "");
			buf_print(b, c_status, rest+1, true);
		break;
		default:
			rest=strtok(NULL, "");
			char umsg[16+strlen(dest)+strlen(rest)];
			sprintf(umsg, "<<%d? %s %s", num, dest, rest);
			buf_print(b, c_unn, umsg, true);
		break;
	}
	return(num);
}

int rx_ping(int fd)
{
	char *sender=strtok(NULL, " ");
	char pong[8+strlen(username)+strlen(sender)];
	sprintf(pong, "PONG %s %s", username, sender+1);
	return(irc_tx(fd, pong));
}

int rx_mode(int fd, bool *join, int b)
{
	if(chan && !*join)
	{
		char joinmsg[8+strlen(chan)];
		sprintf(joinmsg, "JOIN %s", chan);
		irc_tx(fd, joinmsg);
		char jmsg[16+strlen(chan)];
		sprintf(jmsg, "auto-Joining %s", chan);
		buf_print(b, c_join[0], jmsg, true);
		*join=true;
	}
	return(0);
}

int rx_kill(int b, fd_set *master)
{
	int fd=bufs[b].handle;
	char *dest=strtok(NULL, " \t"); // user to be killed
	if(strcmp(dest, bufs[b].nick)==0) // if it's us, we disconnect from the server
	{
		close(fd);
		FD_CLR(fd, master);
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
	else // if it's not us, generate quit messages into the relevant channel tabs
	{
		int b2;
		for(b2=1;b2<nbufs;b2++)
		{
			while((b2<nbufs) && ((bufs[b2].server==b) || (bufs[b2].server==0)))
			{
				if(n_cull(&bufs[b2].nlist, dest))
				{
					char kmsg[24+strlen(dest)+strlen(bufs[b].bname)];
					sprintf(kmsg, "=%s= has left %s (killed)", dest, bufs[b].bname);
					buf_print(b2, c_quit[1], kmsg, true);
				}
			}
		}
	}
	return(0);
}

int rx_error(int b, fd_set *master)
{
	// assume it's fatal
	int fd=bufs[b].handle;
	close(fd);
	FD_CLR(fd, master);
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
	return(redraw_buffer());
}

int rx_privmsg(int b, char *packet, char *pdata)
{
	int fd=bufs[b].handle;
	char *dest=strtok(NULL, " \t");
	char *msg=dest+strlen(dest)+2; // prefixed with :
	char *src=packet+1;
	char *bang=strchr(src, '!');
	if(bang)
		*bang=0;
	char *from=strdup(src);
	src=crush(src, maxnlen);
	int b2;
	bool match=false;
	for(b2=0;b2<nbufs;b2++)
	{
		if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcasecmp(dest, bufs[b2].bname)==0))
		{
			match=true;
			if(*msg==1) // CTCP (TODO: show message for unrecognised CTCP cmds)
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
					char resp[32+strlen(from)+strlen(fname)];
					sprintf(resp, "NOTICE %s \001FINGER :%s\001", from, fname);
					irc_tx(fd, resp);
				}
				else if(strncmp(msg, "\001VERSION", 8)==0)
				{
					char resp[32+strlen(from)+strlen(version)];
					sprintf(resp, "NOTICE %s \001VERSION %s:%s:%s\001", from, "quIRC", version, CC_VERSION);
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
	if(!match) // TODO try matching dest to nick; if that fails print ?? followed by the pdata
	{
		if(strcasecmp(dest, bufs[b].nick)==0)
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
					buf_print(b, c_actn[1], out, true);
					free(out);
				}
				else if(strncmp(msg, "\001FINGER", 7)==0)
				{
					char resp[32+strlen(from)+strlen(fname)];
					sprintf(resp, "NOTICE %s \001FINGER :%s\001", from, fname);
					irc_tx(fd, resp);
				}
				else if(strncmp(msg, "\001VERSION", 8)==0)
				{
					char resp[32+strlen(from)+strlen(version)];
					sprintf(resp, "NOTICE %s \001VERSION %s:%s:%s\001", from, "quIRC", version, CC_VERSION);
					irc_tx(fd, resp);
				}
			}
			else
			{
				char *out=(char *)malloc(16+max(maxnlen, strlen(src)));
				memset(out, ' ', max(maxnlen-strlen(src), 0));
				out[max(maxnlen-strlen(src), 0)]=0;
				sprintf(out+strlen(out), "(from %s) ", src);
				wordline(msg, 9+max(maxnlen, strlen(src)), &out);
				buf_print(b, c_msg[1], out, true);
				free(out);
			}
		}
		else
		{
			char dstr[4+strlen(pdata)];
			sprintf(dstr, "?? %s", pdata);
			buf_print(b, c_err, dstr, true);
		}
	}
	free(from);
	return(0);
}
