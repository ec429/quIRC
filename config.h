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

typedef struct _chanlist
{
	char *name;
	char *key;
	struct _chanlist *next;
	struct _name *igns;
}
chanlist;

typedef struct _servlist
{
	char *name;
	char *portno;
	char *nick;
	bool join;
	chanlist *chans;
	struct _servlist *next;
	struct _name *igns;
}
servlist;

#include "names.h"
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
bool autojoin;
char *username, *fname, *nick, *portno;
servlist *servs;
name *igns;
#ifdef HAVE_DEBUG
int debug;
#endif // HAVE_DEBUG
char version[16+strlen(VERSION_TXT)];

int def_config(void); // set these to their defaults
int rcread(FILE *rcfp); // read & parse the rc file.
signed int pargs(int argc, char *argv[]); // parse the cmdline args.  If return is >=0, main should return it also
void freeservlist(servlist * serv);
void freechanlist(chanlist * chan);
