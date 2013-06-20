/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	process: subprocess & symbiont control
*/

#include <stdbool.h>
#include <sys/types.h>
#include "types.h"

typedef struct
{
	enum {BS_STATUS, BS_ANY, BS_SERVER, BS_CHANNEL, BS_NICK} type; // note that we never store "home 0", we convert it to a definite spec in parse_bufspec
	char *server; // this refers to realsname, not bname, which _might_ not be what you want; no guarantee this won't change.  Scripts should generally only use it as a token anyway
	char *channel; // for BS_NICK this is <nick> not <channel>
	bool warned; // have we previously reported this bufspec bad?
}
bufspec;

bufspec parse_bufspec(const char *spec, bufspec home); // must free_
bufspec clone_bufspec(bufspec spec); // must free_
char *print_bufspec(bufspec spec);
int resolve_bufspec(bufspec spec);
bool match_bufspec(int buf, bufspec spec);
void free_bufspec(bufspec spec);

typedef struct grab_t
{
	bufspec where;
	char *cmd;
	struct grab_t *next;
}
grab_t;

typedef struct listen_t
{
	bufspec where;
	rxmode rx;
	struct listen_t *next;
}
listen_t;

typedef struct
{
	pid_t pid;
	int in, out, err; // pipe fds, for child's stdin/stdout/stderr
	bufspec home; // PRELOAD
	listen_t *listen;
	grab_t *grab;
}
symbiont;

typedef struct symbiont_list
{
	symbiont *car;
	struct symbiont_list *cdr;
}
symbiont_list;

symbiont_list *symbionts;

int fork_symbiont(symbiont *buf, char *const *argvl); // buf should be preloaded with PRELOAD fields
int rx_event(symbiont *sym, rxmode rx, mtype lm, prio lq, char lp, bool ls, const char *lt, const char *ltag, time_t ts);
