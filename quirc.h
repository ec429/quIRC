/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	quirc.h: header for main source file (quirc.c)
*/

#define _GNU_SOURCE // feature test macro

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>

#include "ttyraw.h"
#include "ttyesc.h"
#include "colour.h"
#include "irc.h"
#include "bits.h"
#include "buffer.h"
#include "numeric.h"
#include "config.h"
#include "version.h"
