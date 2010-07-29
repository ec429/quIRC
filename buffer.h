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

typedef struct _name
{
	char *data;
	struct _name *next, *prev;
}
name;

typedef enum
{
	STATUS,
	SERVER,
	CHANNEL,
	PRIVATE // right now we don't have handy private chat, you just have to /msg in a server or channel
}
btype;

typedef struct _buf
{
	btype type;
	char *bname; // "status" or serverloc or #channel or nick (resp. types)
	name *nlist; // only used for channels and private
	int handle; // used for server
	int server; // used by channels and private to denote their 'parent' server.  In server, points to self.  Is an offset into 'bufs'
	char *nick; // used for server
	int nlines;
	int ptr;
	int scroll;
	colour *lc;
	char **lt;
	time_t *ts;
	bool filled;
	bool alert; // tab has new messages?
}
buffer;

int nbufs;
int cbuf;
buffer *bufs;

int init_buffer(int buf, btype type, char *bname, int nlines);
int free_buffer(int buf);
int add_to_buffer(int buf, colour lc, char *lt);
int redraw_buffer(void);
int buf_print(int buf, colour lc, char *lt, bool nl); // don't include trailing \n, because buf_print appends CLR \n
void in_update(char *inp);
