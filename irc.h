#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	irc: networking functions
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

typedef enum
{
	RFC1459,
	STRICT_RFC1459,
	ASCII
}
cmap;

typedef struct
{
	char *prefix;
	char *cmd;
	int nargs;
	char *args[15]; // RFC specifies maximum of 15 args
}
message;

#include "bits.h"
#include "buffer.h"
#include "colour.h"
#include "config.h"
#include "numeric.h"
#include "names.h"

#define MQUOTE	'\020'

int irc_connect(char *server, char *portno, fd_set *master, int *fdmax); // non-blocking
int irc_conn_rest(int b, char *nick, char *username, char *fullname); // call this when the non-blocking connect() has finished
int autoconnect(fd_set *master, int *fdmax, servlist *serv);
int irc_tx(int fd, char * packet);
int irc_rx(int fd, char ** data);

message irc_breakdown(char *packet);
void message_free(message pkt);
void prefix_split(char * prefix, char **src, char **user, char **host);

void low_quote(char *from, char to[512]);
char * low_dequote(char *buf);

char irc_to_upper(char c, cmap casemapping);
char irc_to_lower(char c, cmap casemapping);
int irc_strcasecmp(char *c1, char *c2, cmap casemapping);
int irc_strncasecmp(char *c1, char *c2, int n, cmap casemapping);

// Received-IRC message handlers
int irc_numeric(message pkt, int b);
int rx_ping(message pkt, int b);
int rx_mode(int b); // the first MODE triggers auto-join.  Apart from using it as a trigger, we don't look at modes just yet
int rx_kill(message pkt, int b, fd_set *master);
int rx_kick(message pkt, int b);
int rx_error(message pkt, int b, fd_set *master);
int rx_privmsg(message pkt, int b, bool notice);
int rx_topic(message pkt, int b);
int rx_join(message pkt, int b);
int rx_part(message pkt, int b);
int rx_quit(message pkt, int b);
int rx_nick(message pkt, int b);

int ctcp(char *msg, char *from, char *src, int b2, bool ha); // Handle CTCP (Client-To-Client Protocol) messages (from is crushed-src)
