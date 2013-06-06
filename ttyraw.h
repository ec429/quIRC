/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	ttyraw: set/unset raw mode on tty
*/

#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>

int ttyraw(int fd);
int ttyreset(int fd);
