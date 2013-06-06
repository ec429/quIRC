#pragma once
/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	logging: routines to log activity
*/

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "types.h"

int log_init(FILE *logf, logtype logt);
int log_add(FILE *logf, logtype logt, mtype lm, prio lq, char lp, bool ls, const char *lt, const char *ltag, time_t ts);
