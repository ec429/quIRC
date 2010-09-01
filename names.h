#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	names: handling for name lists
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

typedef struct _name
{
	bool icase; // only used for ignore lists; true = case-insensitive
	bool pms; // only used for ignore lists; true = affect private messages
	char *data; // is a unique pointer (eg. from strdup()), and must be free()d
	struct _name *next, *prev;
}
name;

name * n_add(name ** list, char *data); // returns pointer to the added name (which should also be the new value of *list).  Calls n_cull() first
int n_cull(name ** list, char *data); // returns number of instances culled
void n_free(name * list);

int i_match(name * list, char *nm, bool pm); // returns number of matches
int i_cull(name ** list, char *nm); // returns number of instances culled
