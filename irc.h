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
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "bits.h"
#include "buffer.h"
#include "colour.h"
#include "config.h"
#include "numeric.h"

int irc_connect(char *server, char *portno, char *nick, char *username, char *fullname, fd_set *master, int *fdmax);
int autoconnect(fd_set *master, int *fdmax);
int irc_tx(int fd, char * packet);
int irc_rx(int fd, char ** data);

// Received-IRC message handlers.  strtok() state leaks across the boundaries of these functions, beware!
int irc_numeric(char *cmd, int b);
int rx_ping(int fd);
int rx_mode(int fd, bool *join, int b); // the first MODE triggers auto-join.  Apart from using it as a trigger, we don't look at modes just yet
int rx_kill(int b, fd_set *master);
int rx_error(int b, fd_set *master);
int rx_privmsg(int b, char *packet, char *pdata);
