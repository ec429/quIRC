/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	buffer: multiple-buffer control
*/

typedef enum
{
	STATUS,
	SERVER,
	CHANNEL
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
}
buffer;

buffer *bufs;

int init_buffer(buffer *buf, btype type, char *bname, int nlines);
int add_to_buffer(buffer *buf, colour lc, char *lt, time_t ts);
