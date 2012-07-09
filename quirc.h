/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	quirc.h: header for main source file (quirc.c)
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#ifdef	USE_MTRACE
	#include <mcheck.h>
#endif // USE_MTRACE

#include "ttyraw.h"
#include "ttyesc.h"
#include "colour.h"
#include "irc.h"
#include "bits.h"
#include "buffer.h"
#include "config.h"
#include "input.h"
#include "version.h"
