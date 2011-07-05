#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-11 Edward Cree

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
#include "buffer.h"

// global settings & state
#include "config_globals.h"
bool autojoin;
char *username, *fname, *nick, *portno;
servlist *servs;
name *igns;
char *version;

int conf_check(void); // writes diagnostics to start-buffer
int def_config(void); // set these to their defaults
int rcread(FILE *rcfp); // read & parse the rc file.
signed int pargs(int argc, char *argv[]); // parse the cmdline args.  If return is >=0, main should return it also
void freeservlist(servlist * serv);
void freechanlist(chanlist * chan);
