/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	types.c: enumerator names
*/

#include "types.h"

// TODO: The _name functions ought to be generated, really (SPOT and all that)

const char *mtype_name(mtype m)
{
	switch(m)
	{
		case MSG:
			return("MSG");
		case NOTICE:
			return("NOTICE");
		case PREFORMAT:
			return("PREFORMAT");
		case ACT:
			return("ACT");
		case JOIN:
			return("JOIN");
		case PART:
			return("PART");
		case QUIT:
			return("QUIT");
		case QUIT_PREFORMAT:
			return("QUIT_PREFORMAT");
		case NICK:
			return("NICK");
		case MODE:
			return("MODE");
		case STA:
			return("STA");
		case ERR:
			return("ERR");
		case UNK:
			return("UNK");
		case UNK_NOTICE:
			return("UNK_NOTICE");
		case UNN:
			return("UNN");
		default:
			return("?");
	}
}

const char *prio_name(prio p)
{
	switch(p)
	{
		case QUIET:
			return("QUIET");
		case NORMAL:
			return("NORMAL");
		case DEBUG:
			return("DEBUG");
		default:
			return("?");
	}
}
