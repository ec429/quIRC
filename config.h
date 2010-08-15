/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	config: rc file and option parsing
*/

#include <stdio.h>
#include <stdbool.h>
#include "bits.h"
#include "colour.h"
#include "text.h"
#include "version.h"

int rcread(FILE *rcfp, char **server, char **portno, char **uname, char **fname, char **nick, char **chan, int *maxnlen, int *buflines); // read & parse the rc file
signed int pargs(int argc, char *argv[], char **server, char **portno, char **uname, char **fname, char **nick, char **chan, int *maxnlen, int *buflines); // parse the cmdline args.  If return is >=0, main should return it also
