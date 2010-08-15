#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	names: handling for name lists
*/

#include <stdlib.h>
#include <string.h>

typedef struct _name
{
	char *data; // is a unique pointer (eg. from strdup()), and must be free()d
	struct _name *next, *prev;
}
name;

name * n_add(name ** list, char *data); // returns pointer to the added name (which should also be the new value of *list).  Calls n_cull() first
int n_cull(name ** list, char *data); // returns number of instances culled
void n_free(name * list);
