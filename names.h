#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	names: handling for name lists
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

typedef struct
{
	char letter;
	char pfx;
}
prefix;

typedef struct _name
{
	bool icase; // only used for ignore lists; true = case-insensitive
	bool pms; // only used for ignore lists; true = affect private messages
	unsigned int npfx;
	prefix *prefixes;
	char *data; // is a unique pointer (eg. from strdup()), and must be free()d
	struct _name *next, *prev;
}
name;

#include "buffer.h"
#include "irc.h"

name * n_add(name ** list, const char *data, cmap casemapping); // returns pointer to the added name.  Calls n_cull() first
int n_cull(name ** list, const char *data, cmap casemapping); // returns number of instances culled
void n_free(name * list);

int i_match(name * list, const char *nm, bool pm, cmap casemapping); // returns number of matches
int i_cull(name ** list, const char *nm); // returns number of instances culled
void i_list(void);
