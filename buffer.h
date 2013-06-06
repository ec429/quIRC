#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	buffer: multiple-buffer control & buffer rendering
*/

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include "colour.h"
#include "config.h"
#include "input.h"
#include "irc.h"
#include "types.h"

#define SERVER(buf)	(bufs[bufs[buf].server]) // server of a buf
#define LIVE(buf)	(bufs[buf].live && SERVER(buf).live)	// Check liveness
#define STAMP_LEN	40

typedef struct _buf
{
	btype type;
	char *bname; // Buffer display name: "status" or serverloc(or NETWORK) or #channel or @nick (resp. types)
	char *serverloc; // address of server roundrobin
	char *realsname; // real server name (not the same as bname)
	name *nlist; // only used for BT_CHANNEL and BT_PRIVATE: linked-list of nicks
	name *us; // pointer to our entry in the nlist
	name *ilist; // ignore-list
	int handle; // used for BT_SERVER: file descriptor
	int server; // used by BT_CHANNEL and BT_PRIVATE to denote their 'parent' server.  In BT_SERVER||BT_STATUS, points to self.  Is an offset into 'bufs'
	char *nick; // used for BT_SERVER: user's nick on this server
	char *topic; // used for BT_CHANNEL
	FILE *logf;
	logtype logt;
	int nlines; // number of lines allocated
	int ptr; // pointer to current unproc line
	int scroll; // unproc line of screen bottom (which is one physical line below the last displayed text)
	int ascroll; // physical line within [scroll]
	bool *ls; // array of whether line was sent or rxed
	prio *lq; // array of priority levels for lines
	mtype *lm; // array of message types for lines
	char *lp; // array of prefix chars (0 for use default)
	char **lt; // array of (unprocessed) text for lines
	char **ltag; // array of (unprocessed, uncrushed) tag text for lines
	time_t *ts; // array of timestamps for unproc lines
	bool filled; // buffer has filled up and looped? (the buffers are circular in nature)
	bool dirty; // processed lines are out of date? (TODO: make this indicate /which/ are out of date and only re-render those)
	int *lpl; // count of processed lines for each line
	colour *lpc; // array of colours for each line
	char ***lpt; // array of processed lines for each line
	bool alert; // tab has new messages?
	int hi_alert; // high-level alert status: 0 = none; 1: on (if alert then flashing else single flash); 2: off (flashing)
	int ping; // ping/idleness status (BT_SERVER)
	time_t last; // when was the last RX? (BT_SERVER)
	bool namreply; // tab is in the middle of reading a list of NAMES replies (RPL_NAMREPLY)?
	bool live; // tab is connected?  when checking in a BT_CHANNEL, remember to AND it with the parent's live (use LIVE(buf), defined further up this file)
	bool conninpr; // connection in progress? (BT_SERVER only)
	ibuffer input; // input history
	cmap casemapping; // the BT_SERVER's value is authoritative; the BT_CHANNEL's value is ignored.  BT_STATUS's value is irrelevant.  Set by ISUPPORT
	unsigned int npfx;// the BT_SERVER's value denotes the available list (set by ISUPPORT); the BT_CHANNEL's value lists the modes set on that channel (letter only)
	prefix *prefixes; // ^^
	servlist * autoent; // if this was opened by autoconnect(), this is filled in to point to the server's servlist entry
	bool conf; // Conference Mode (hides joins, parts, quits, and /nicks)
	char *key; // channel key
	char *lastkey; // last key passed to /rejoin; copied to .key if rejoin succeeds
}
buffer;

int nbufs;
int cbuf;
buffer *bufs;

typedef struct
{
	int nlines;
	int ptr;
	bool filled;
	char **lt;
	char **ltag;
	mtype *lm;
	time_t *ts;
	bool loop;
	int errs;
}
ring;

ring s_buf, d_buf;

int init_ring(ring *r);
int add_to_ring(ring *r, mtype lm, const char *lt, const char *ltag);
int atr_failsafe(ring *r, mtype lm, const char *lt, const char *ltag);
int free_ring(ring *r);
int initialise_buffers(int buflines);
int init_buffer(int buf, btype type, const char *bname, int nlines);
int free_buffer(int buf);
int add_to_buffer(int buf, mtype lm, prio lq, char lp, bool ls, const char *lt, const char *ltag);
int mark_buffer_dirty(int buf);
int redraw_buffer(void);
int render_buffer(int buf);
int render_line(int buf, int uline);
int e_buf_print(int buf, mtype lm, message pkt, const char *lead);
int transfer_ring(ring *r, prio lq);
int push_ring(ring *r, prio lq);
void in_update(iline inp);
void titlebar(void);
int findptab(int b, const char *src);
int makeptab(int b, const char *src);
void timestamp(char stamp[STAMP_LEN], time_t t);
bool isutf8(const char *src, size_t *len); // determine if a string starts with a non-ASCII UTF8 character; if so, give its length (in bytes) in len.  If this function returns false, the value of *len is undefined
