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
void low_quote(char *from, char to[512]);
char * low_dequote(char *buf);

char irc_to_upper(char c, cmap casemapping);
char irc_to_lower(char c, cmap casemapping);
int irc_strcasecmp(char *c1, char *c2, cmap casemapping);
int irc_strncasecmp(char *c1, char *c2, int n, cmap casemapping);

// Received-IRC message handlers.  strtok() state leaks across the boundaries of these functions, beware!
int irc_numeric(char *cmd, int b);
int rx_ping(int fd);
int rx_mode(servlist * serv, int b); // the first MODE triggers auto-join.  Apart from using it as a trigger, we don't look at modes just yet
int rx_kill(int b, fd_set *master);
int rx_kick(int b);
int rx_error(int b, fd_set *master);
int rx_privmsg(int b, char *packet, char *pdata);
int rx_notice(int b, char *packet);
int rx_topic(int b, char *packet);
int rx_join(int b, char *packet, char *pdata);
int rx_part(int b, char *packet, char *pdata);
int rx_quit(int b, char *packet, char *pdata);
int rx_nick(int b, char *packet, char *pdata);

int ctcp(char *msg, char *from, char *src, int b2); // Handle CTCP (Client-To-Client Protocol) messages (from is crushed-src)
