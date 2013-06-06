/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	ttyraw: set/unset raw mode on tty

	Based on http://docs.linux.cz/programming/c/unix_examples/raw.html
*/

#include "ttyraw.h"

struct termios oldtermios;

int ttyraw(int fd)
{
	/*	Set terminal mode as follows:
		Noncanonical mode - turn off ICANON.
		Disable signal generation - turn off ISIG
		Turn ECHO mode off.
		Disable input parity detection (INPCK).
		Disable stripping of eighth bit on input (ISTRIP).
		Disable flow control (IXON).
		Use eight bit characters (CS8).
		Disable parity checking (PARENB).
		One byte at a time input (MIN=1, TIME=0).
	*/
	struct termios newtermios;
	if(tcgetattr(fd, &oldtermios) < 0)
		return(-1);
	newtermios = oldtermios;

	newtermios.c_lflag &= ~(ICANON | ISIG | ECHO);
	newtermios.c_iflag &= ~(INPCK | ISTRIP | IXON);
	newtermios.c_cflag &= ~(CSIZE | PARENB);
	newtermios.c_cflag |= CS8;
	newtermios.c_cc[VMIN] = 1;
	newtermios.c_cc[VTIME] = 0;
	if(tcsetattr(fd, TCSAFLUSH, &newtermios) < 0)
		return(-1);
	return(0);
}

int ttyreset(int fd)
{
	if(tcsetattr(fd, TCSAFLUSH, &oldtermios) < 0)
		return(-1);
	return(0);
}
