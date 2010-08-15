#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	input: handle input routines
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ttyesc.h"
#include "names.h"
#include "buffer.h"

int inputchar(char **inp, int *state);
