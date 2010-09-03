#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	buffer: multiple-buffer control
*/

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "ttyesc.h"
#include "colour.h"
#include "config.h"
#include "bits.h"
#include "irc.h"
#include "names.h"
#include "text.h"
#include "version.h"

#define LIVE(buf)	(bufs[buf].live && bufs[bufs[buf].server].live)	// Check liveness

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
	char *bname; // "status" or serverloc or #channel or @nick (resp. types)
	name *nlist; // only used for channels and private: linked-list of nicks
	name *ilist; // ignore-list
	int handle; // used for server: file descriptor
	int server; // used by channels and private to denote their 'parent' server.  In server, points to self.  Is an offset into 'bufs'
	char *nick; // used for server: user's nick on this server
	int nlines; // number of lines allocated
	int ptr; // pointer to current line
	int scroll; // current scroll position (distance up from ptr)
	colour *lc; // array of colours for lines
	char **lt; // array of text for lines
	time_t *ts; // array of timestamps for lines (not used now, but there ready for eg. mergebuffers)
	bool filled; // buffer has filled up and looped? (the buffers are circular in nature)
	bool alert; // tab has new messages?
	bool namreply; // tab is in the middle of reading a list of NAMES replies (RPL_NAMREPLY)?
	bool live; // tab is connected?  when checking in a CHANNEL, remember to AND it with the parent's live (use LIVE(buf), defined further up this file)
	cmap casemapping; // the SERVER's value is authoritative; the CHANNEL's value is ignored.  STATUS's value is irrelevant.
}
buffer;

int nbufs;
int cbuf;
buffer *bufs;

int initialise_buffers(int buflines, char *nick);
int init_buffer(int buf, btype type, char *bname, int nlines);
int free_buffer(int buf);
int add_to_buffer(int buf, colour lc, char *lt);
int redraw_buffer(void);
int buf_print(int buf, colour lc, char *lt); // don't include trailing \n, because buf_print appends CLR \n
int w_buf_print(int buf, colour lc, char *lt, char *lead);
void in_update(char *inp);
char *highlight(char *src); // use ANSI-colours to highlight \escapes.  Returns a malloc-like pointer
void titlebar(void);
