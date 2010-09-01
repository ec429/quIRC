/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	irc: networking functions
*/

#include "irc.h"

int irc_connect(char *server, char *portno, fd_set *master, int *fdmax)
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
		char *err=(char *)gai_strerror(rv);
		w_buf_print(0, c_err, err, "getaddrinfo: ");
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
			w_buf_print(0, c_err, strerror(errno), "socket: ");
			continue;
		}
		if(fcntl(serverhandle, F_SETFD, O_NONBLOCK) == -1)
		{
			close(serverhandle);
			w_buf_print(0, c_err, strerror(errno), "fcntl: ");
			continue;
		}
		if(connect(serverhandle, p->ai_addr, p->ai_addrlen) == -1)
		{
			if(errno!=EINPROGRESS)
			{
				close(serverhandle);
				w_buf_print(0, c_err, strerror(errno), "connect: ");
				continue;
			}
		}
		break;
	}
	if (p == NULL)
	{
		w_buf_print(0, c_err, "failed to connect to server", "/connect: ");
		return(0); // 0 indicates failure as rv is new serverhandle value
	}
	freeaddrinfo(servinfo); // don't need this any more
	
	FD_SET(serverhandle, master);
	*fdmax=max(*fdmax, serverhandle);
	return(serverhandle);
}

int irc_conn_rest(int b, char *nick, char *username, char *fullname)
{
	bufs[b].live=true; // mark it as live
	char nickmsg[6+strlen(nick)];
	sprintf(nickmsg, "NICK %s", nick);
	irc_tx(bufs[b].handle, nickmsg);
	struct utsname arch;
	uname(&arch);
	char usermsg[12+strlen(username)+strlen(arch.nodename)+strlen(fullname)];
	sprintf(usermsg, "USER %s %s %s :%s", username, arch.nodename, arch.nodename, fullname);
	irc_tx(bufs[b].handle, usermsg);
	return(0);
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
	int serverhandle=irc_connect(server, portno, master, fdmax);
	if(serverhandle)
	{
		bufs=(buffer *)realloc(bufs, ++nbufs*sizeof(buffer));
		init_buffer(1, SERVER, server, buflines);
		cbuf=1;
		bufs[cbuf].handle=serverhandle;
		bufs[cbuf].nick=strdup(nick);
		bufs[cbuf].server=1;
		add_to_buffer(1, c_status, cstr);
		sprintf(cstr, "quIRC - connecting to %s", server);
		settitle(cstr);
	}
	return(serverhandle);
}

int irc_tx(int fd, char * packet)
{
	//printf(">> %s\n\n", packet); // for debugging purposes
	char pq[512];
	low_quote(packet, pq);
	unsigned long l=min(strlen(pq), 511);
	unsigned long p=0;
	while(p<l)
	{
		signed long j=send(fd, pq+p, l-p, 0);
		if(j<1)
			return(p); // Something went wrong with send()!
		p+=j;
	}
	send(fd, "\n", 1, 0);
	return(l); // Return the number of bytes sent
}

int irc_rx(int fd, char ** data)
{
	char buf[512];
	int l=0;
	bool cr=false;
	while(!cr)
	{
		long bytes=recv(fd, buf+l, 1, MSG_WAITALL);
		if(bytes>0)
		{
			char c=buf[l];
			if((strchr("\n\r", c)!=NULL) || (l>510))
			{
				cr=true;
				buf[l]=0;
			}
			l++;
		}
		else if(bytes<0)
		{
			int b;
			for(b=0;b<nbufs;b++)
			{
				if((fd==bufs[b].handle) && (bufs[b].type==SERVER))
				{
					w_buf_print(b, c_err, strerror(errno), "irc_rx: recv:");
					bufs[b].live=false;
				}
			}
			cr=true; // just crash out with a partial message
			buf[l]=0;
		}
	}
	*data=low_dequote(buf);
	if(!*data)
		return(1);
	return(0);
}

void low_quote(char *from, char to[512])
{
	int o=0;
	while((*from) && (o<510))
	{
		char c=*from++;
		switch(c)
		{
			case '\n':
				to[o++]=MQUOTE;
				to[o++]='n';
			break;
			case '\r':
				to[o++]=MQUOTE;
				to[o++]='r';
			break;
			case MQUOTE:
				to[o++]=MQUOTE;
				to[o++]=MQUOTE;
			break;
			case '\\':
				if(*from=='0') // "\\0", is an encoded '\0'
				{
					to[o++]=MQUOTE; // because this will produce ^P 0, the proper representation
				}
				else
				{
					to[o++]=c;
				}
			break;
			case 0: // can't happen right now
				to[o++]=MQUOTE;
				to[o++]='0';
			break;
			default:
				to[o++]=c;
			break;
		}
	}
	to[o]=0;
}

char * low_dequote(char *buf)
{
	char *rv=(char *)malloc(512);
	if(!rv) return(NULL);
	char *p=buf;
	int o=0;
	while((*p) && (o<510))
	{
		if(*p==MQUOTE)
		{
			char c=*++p;
			switch(c)
			{
				case '0':
					rv[o++]='\\';
					rv[o]='0'; // We will have to defer '\0' handling as we can't stick '\0's in char *s (NUL terminated strings)
				break;
				case 'n':
					rv[o]='\n';
				break;
				case 'r':
					rv[o]='\r';
				break;
				case MQUOTE: // MQUOTE MQUOTE => MQUOTE, so fall through
				default:
					rv[o]=c;
				break;
			}
		}
		else
		{
			rv[o]=*p;
		}
		p++;o++;
	}
	rv[o]=0;
	return(rv);
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
						if(!isalpha(*nn))
							nn++;
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
					w_buf_print(b2, c_status, lmsg, "");
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
			w_buf_print(b, c_notice[1], motdline, "");
		break;
		case ERR_NOMOTD: // 422 <dest> :MOTD File is missing
			rest=strtok(NULL, "");
			w_buf_print(b, c_notice[1], rest+1, "");
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
					w_buf_print(b2, c_notice[1], tmsg, "");
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
					w_buf_print(b2, c_notice[1], tmsg, "");
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
					w_buf_print(b2, c_status, tmsg, "");
				}
			}
		break;
		case RPL_LUSERCLIENT: // 251 <dest> :There are <integer> users and <integer> invisible on <integer> servers"
		case RPL_LUSERME: // 255 <dest> ":I have <integer> clients and <integer> servers
			rest=strtok(NULL, "");
			w_buf_print(b, c_status, rest+1, ": ");
		break;
		case RPL_LUSEROP: // 252 <dest> <integer> :operator(s) online
		case RPL_LUSERUNKNOWN: // 253 <dest> "<integer> :unknown connection(s)
		case RPL_LUSERCHANNELS: // 254 <dest> "<integer> :channels formed
			{
				char *count=strtok(NULL, " ");
				rest=strtok(NULL, "");
				char lmsg[2+strlen(count)+strlen(rest)];
				sprintf(lmsg, "%s %s", count, rest+1);
				w_buf_print(b, c_status, lmsg, ": ");
			}
		break;
		case RPL_X_LOCALUSERS: // 265 <dest> :Current Local Users: <integer>\tMax: <integer>
		case RPL_X_GLOBALUSERS: // 266 <dest> :Current Global Users: <integer>\tMax: <integer>
			rest=strtok(NULL, "");
			w_buf_print(b, c_status, rest+1, ": ");
		break;
		default:
			rest=strtok(NULL, "");
			char umsg[16+strlen(dest)+strlen(rest)];
			sprintf(umsg, "<<%d? %s %s", num, dest, rest);
			w_buf_print(b, c_unn, umsg, "");
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

int rx_mode(bool *join, int b)
{
	int fd=bufs[b].handle;
	if(chan && !*join)
	{
		char joinmsg[8+strlen(chan)];
		sprintf(joinmsg, "JOIN %s", chan);
		irc_tx(fd, joinmsg);
		char jmsg[16+strlen(chan)];
		sprintf(jmsg, "auto-Joining %s", chan);
		w_buf_print(b, c_join[0], jmsg, "");
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
			if((bufs[b2].server==b) || (bufs[b2].server==0))
			{
				bufs[b2].live=false;
			}
		}
		redraw_buffer();
	}
	else // if it's not us, generate quit messages into the relevant channel tabs
	{
		int b2;
		for(b2=1;b2<nbufs;b2++)
		{
			if((bufs[b2].server==b) || (bufs[b2].server==0))
			{
				if(n_cull(&bufs[b2].nlist, dest))
				{
					char kmsg[24+strlen(dest)+strlen(bufs[b].bname)];
					sprintf(kmsg, "=%s= has left %s (killed)", dest, bufs[b].bname);
					w_buf_print(b2, c_quit[1], kmsg, "");
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
		if((bufs[b2].server==b) || (bufs[b2].server==0))
		{
			bufs[b2].live=false;
		}
	}
	return(redraw_buffer());
}

int rx_privmsg(int b, char *packet, char *pdata)
{
	char *dest=strtok(NULL, " \t");
	char *msg=dest+strlen(dest)+2; // prefixed with :
	char *src=packet+1;
	char *bang=strchr(src, '!');
	if(bang)
		*bang=0;
	char *from=strdup(src);
	crush(&from, maxnlen);
	int b2;
	bool match=false;
	for(b2=0;b2<nbufs;b2++)
	{
		if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcasecmp(dest, bufs[b2].bname)==0))
		{
			match=true;
			if(*msg==1) // CTCP
			{
				ctcp(msg, from, src, b2);
			}
			else
			{
				char tag[maxnlen+4]; // TODO this tag-making bit ought to be refactored really
				memset(tag, ' ', maxnlen+3);
				sprintf(tag+maxnlen-strlen(from), "<%s> ", from);
				w_buf_print(b2, c_msg[1], msg, tag);
			}
		}
	}
	if(!match)
	{
		if(strcasecmp(dest, bufs[b].nick)==0)
		{
			if(*msg==1) // CTCP
			{
				ctcp(msg, from, src, b);
			}
			else
			{
				char tag[maxnlen+9];
				memset(tag, ' ', maxnlen+8);
				sprintf(tag+maxnlen-strlen(from), "(from %s) ", from);
				w_buf_print(b, c_msg[1], msg, tag);
			}
		}
		else
		{
			w_buf_print(b, c_err, pdata, "?? ");
		}
	}
	free(from);
	return(0);
}

int rx_notice(int b, char *packet)
{
	char *dest=strtok(NULL, " ");
	char *msg=dest+strlen(dest)+2; // prefixed with :
	char *src=packet+1;
	char *bang=strchr(src, '!');
	if(bang)
		*bang=0;
	char *from=strdup(src);
	scrush(&from, maxnlen);
	char tag[maxnlen+9];
	memset(tag, ' ', maxnlen+8);
	sprintf(tag+maxnlen-strlen(from), "(from %s) ", from);
	return(w_buf_print(b, c_notice[1], msg, tag));
}

int rx_topic(int b, char *packet)
{
	// <dest> :<topic>
	char *dest=strtok(NULL, " ");
	char *msg=dest+strlen(dest)+2; // prefixed with :
	char *src=packet+1;
	char *bang=strchr(src, '!');
	if(bang)
		*bang=0;
	char *from=strdup(src);
	scrush(&from, maxnlen);
	char tag[maxnlen+20];
	sprintf(tag, "%s set the Topic to ", from);
	int b2;
	bool match=false;
	for(b2=0;b2<nbufs;b2++)
	{
		if((bufs[b2].server==b) && (bufs[b2].type==CHANNEL) && (strcasecmp(dest, bufs[b2].bname)==0))
		{
			w_buf_print(b2, c_notice[1], msg, tag);
			match=true;
		}
	}
	return(match?0:1);
}

int rx_join(int b, char *packet, char *pdata, bool *join)
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
		*join=true;
		char cstr[16+strlen(src)+strlen(bufs[b].bname)];
		sprintf(cstr, "quIRC - %s on %s", src, bufs[b].bname);
		settitle(cstr);
		bufs=(buffer *)realloc(bufs, ++nbufs*sizeof(buffer));
		init_buffer(nbufs-1, CHANNEL, chan, buflines);
		bufs[nbufs-1].server=bufs[b].server;
		cbuf=nbufs-1;
		w_buf_print(cbuf, c_join[1], dstr, "");
		bufs[cbuf].handle=bufs[bufs[cbuf].server].handle;
		bufs[cbuf].live=true;
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
				w_buf_print(b2, c_join[1], dstr, "");
				n_add(&bufs[b2].nlist, src);
			}
		}
		if(!match)
		{
			w_buf_print(b, c_err, pdata, "?? ");
		}
	}
	return(0);
}

int rx_part(int b, char *packet, char *pdata)
{
	char *dest=strtok(NULL, " \t");
	char *src=packet+1;
	char *bang=strchr(src, '!');
	if(bang)
		*bang=0;
	if(strcmp(src, bufs[b].nick)==0)
	{
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
				w_buf_print(b2, c_part[1], dstr, "");
				n_cull(&bufs[b2].nlist, src);
			}
		}
		if(!match)
		{
			w_buf_print(b, c_err, pdata, "?? ");
		}
	}
	return(0);
}

int rx_quit(int b, char *packet, char *pdata)
{
	char *dest=strtok(NULL, "");
	char *src=packet+1;
	char *bang=strchr(src, '!');
	if(bang)
		*bang=0;
	if(strcmp(src, bufs[b].nick)==0) // this shouldn't happen
	{
		w_buf_print(b, c_err, pdata, "?? ");
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
					w_buf_print(b2, c_quit[1], dstr, "");
				}
			}
		}
	}
	return(0);
}

int rx_nick(int b, char *packet, char *pdata)
{
	char *dest=strtok(NULL, " \t");
	char *src=packet+1;
	char *bang=strchr(src, '!');
	if(bang)
		*bang=0;
	if(strcmp(dest+1, bufs[b].nick)==0)
	{
		char dstr[30+strlen(src)+strlen(dest+1)];
		sprintf(dstr, "You (%s) are now known as %s", src, dest+1);
		int b2;
		for(b2=0;b2<nbufs;b2++)
		{
			if(bufs[b2].server==b)
			{
				w_buf_print(b2, c_nick[1], dstr, "");
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
					w_buf_print(b2, c_nick[1], dstr, "");
				}
			}
		}
		if(!match)
		{
			w_buf_print(b, c_err, pdata, "?? ");
		}
	}
	return(0);
}

int ctcp(char *msg, char *from, char *src, int b2)
{
	int fd=bufs[b2].handle;
	if(strncmp(msg, "\001ACTION ", 8)==0)
	{
		msg[strlen(msg)-1]=0; // remove trailing \001
		char tag[maxnlen+4];
		memset(tag, ' ', maxnlen+3);
		sprintf(tag+maxnlen+2-strlen(from), "%s ", from);
		w_buf_print(b2, c_actn[1], msg+8, tag);
	}
	else if(strncmp(msg, "\001FINGER", 7)==0)
	{
		char resp[32+strlen(src)+strlen(fname)];
		sprintf(resp, "NOTICE %s \001FINGER :%s\001", src, fname);
		irc_tx(fd, resp);
	}
	else if(strncmp(msg, "\001VERSION", 8)==0)
	{
		char resp[32+strlen(src)+strlen(version)+strlen(CC_VERSION)];
		sprintf(resp, "NOTICE %s \001VERSION %s:%s:%s\001", src, "quIRC", version, CC_VERSION);
		irc_tx(fd, resp);
	}
	else
	{
		char tag[maxnlen+9];
		memset(tag, ' ', maxnlen+8);
		sprintf(tag+maxnlen-strlen(from), "(from %s) ", from);
		char *cmd=msg+1;
		char *space=strchr(cmd, ' ');
		if(space)
			*space=0;
		char cmsg[32+strlen(cmd)];
		sprintf(cmsg, "Unrecognised CTCP %s (ignoring)", cmd);
		w_buf_print(b2, c_unk, cmsg, tag);
	}
	return(0);
}
