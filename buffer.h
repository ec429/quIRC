/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	buffer: multiple-buffer control
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ttyesc.h"
#include "colour.h"
#include "bits.h"

typedef enum
{
	STATUS,
	SERVER,
	CHANNEL,
	PRIVATE
}
btype;

typedef struct
{
	btype type;
	char *bname;
	int nlines;
	int ptr;
	colour *lc;
	char **lt;
	time_t *ts;
	bool filled;
}
buffer;

buffer *bufs;

int init_buffer(buffer *buf, btype type, char *bname, int nlines);
int add_to_buffer(buffer *buf, colour lc, char *lt);
int buf_print(buffer *buf, colour lc, char *lt, bool nl); // don't include trailing \n, because buf_print appends CLR \n
