#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	types.h: define data types that lots of modules need
*/

typedef enum
{
	LOGT_PLAIN,
	LOGT_SYMBOLIC,
}
logtype;

typedef enum
{
	STATUS,
	SERVER,
	CHANNEL,
	PRIVATE
}
btype;

typedef enum
{
	MSG,
	NOTICE,
	PREFORMAT,
	ACT,
	JOIN,
	PART,
	QUIT,
	QUIT_PREFORMAT,
	NICK,
	MODE,
	STA,
	ERR,
	UNK,
	UNK_NOTICE,
	UNN,
}
mtype;
const char *mtype_name(mtype m);

typedef enum
{
	QUIET,	// hide in quiet mode
	NORMAL,	// always show
	DEBUG,	// show only in debug mode
}
prio;
const char *prio_name(prio p);
