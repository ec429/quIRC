#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	config: rc file and option parsing
*/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define CLIENT_SOURCE	"http://github.com/ec429/quIRC"

#include "types.h"

typedef struct _chanlist
{
	char *name;
	char *key;
	FILE *logf;
	logtype logt;
	struct _chanlist *next;
	struct _name *igns;
}
chanlist;

typedef struct _servlist
{
	char *name;
	char *portno;
	char *nick;
	char *pass;
	bool join;
	chanlist *chans;
	struct _servlist *next;
	struct _name *igns;
}
servlist;

#include "names.h"
#include "keymod.h"

// global settings & state
#include "config_globals.h"
extern bool autojoin;
extern char *username, *fname, *nick, *pass, *portno;
extern bool defnick;
extern servlist *servs;
extern name *igns;
extern char *version;
extern unsigned int nkeys;
extern keymod *kmap;

int initkeys(void);
void loadkeys(FILE *);
int conf_check(void); // writes diagnostics to start-buffer
int def_config(void); // set these to their defaults
int rcread(FILE *rcfp); // read & parse the rc file.
signed int pargs(int argc, char *argv[]); // parse the cmdline args.  If return is >=0, main should return it also
void freeservlist(servlist *serv);
void freechanlist(chanlist *chan);
