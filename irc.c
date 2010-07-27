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
	//printf("Looking up server: %s:%s\n", server, portno);
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
	//printf("running, connecting to server...\n");
	struct addrinfo *p;
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		inet_ntop(p->ai_family, &(((struct sockaddr_in*)p->ai_addr)->sin_addr), sip, sizeof(sip));
		//printf("connecting to %s\n", sip);
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
	freeaddrinfo(servinfo); // all done with this structure
	
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
