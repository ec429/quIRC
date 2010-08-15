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
int irc_numeric(char *cmd, int b); // strtok() state leaks across the boundaries of this function, beware!
