#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	irc: networking functions
*/

#define _GNU_SOURCE // feature test macro

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
#include <signal.h>

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
#include "osconf.h"

#define MQUOTE	'\020'

volatile sig_atomic_t sigpipe, sigwinch, sigusr1;

void handle_signals(int); // handles sigpipe, sigwinch and sigusr1

#if ASYNCH_NL
typedef struct _nl_list
{
	struct gaicb *nl_details;
	int reconn_b;
	servlist *autoent; // filled out by autoconnect
	struct _nl_list *prev, *next;
}
nl_list;
nl_list *nl_active;
nl_list *irc_connect(char *server, const char *portno); // non-blocking NL
int irc_conn_found(nl_list **list, fd_set *master, int *fdmax); // non-blocking; call this when the getaddrinfo_a() has finished
#else
int irc_connect(char *server, const char *portno, fd_set *master, int *fdmax); // non-blocking
#endif
int irc_conn_rest(int b, char *nick, char *username, char *passwd, char *fullname); // call this when the non-blocking connect() has finished
int autoconnect(fd_set *master, int *fdmax, servlist *serv);
int irc_tx(int fd, char * packet);
int irc_rx(int fd, char ** data, fd_set *master);

message irc_breakdown(char *packet);
void message_free(message pkt);
void prefix_split(char * prefix, char **src, char **user, char **host);

void low_quote(char *from, char to[512]);
char * low_dequote(char *buf);

char irc_to_upper(char c, cmap casemapping);
char irc_to_lower(char c, cmap casemapping);
int irc_strcasecmp(const char *c1, const char *c2, cmap casemapping);
int irc_strncasecmp(const char *c1, const char *c2, int n, cmap casemapping);

// Send an IRC PRIVMSG (ie. ordinary talking)
int talk(char *iinput);

// Received-IRC message handlers
int irc_numeric(message pkt, int b);
int rx_ping(message pkt, int b);
int rx_mode(message pkt, int b); // the first MODE triggers auto-join
int rx_kill(message pkt, int b, fd_set *master);
int rx_kick(message pkt, int b);
int rx_error(message pkt, int b, fd_set *master);
int rx_privmsg(message pkt, int b, bool notice);
int rx_topic(message pkt, int b);
int rx_join(message pkt, int b);
int rx_part(message pkt, int b);
int rx_quit(message pkt, int b);
int rx_nick(message pkt, int b);

int ctcp_strip(char *msg, const char *src, int b2, bool ha, bool notice, bool priv, bool tx); // Strip the CTCPs out of a privmsg and handle them
int ctcp(const char *msg, const char *src, int b2, bool ha, bool notice, bool priv, bool tx); // Handle CTCP (Client-To-Client Protocol) messages
