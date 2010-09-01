#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	config: rc file and option parsing
*/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "bits.h"
#include "colour.h"
#include "text.h"
#include "version.h"

// global settings & state
int width, height; // term size
int mirc_colour_compat;
int force_redraw;
int buflines;
int maxnlen;
bool full_width_colour;
bool hilite_tabstrip;
bool tsb; // top status bar
char *server, *portno, *username, *fname, *nick, *chan;
char version[16+strlen(VERSION_TXT)];

int def_config(void); // set these to their defaults
int rcread(FILE *rcfp); // read & parse the rc file.
signed int pargs(int argc, char *argv[]); // parse the cmdline args.  If return is >=0, main should return it also
