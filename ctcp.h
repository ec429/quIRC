#pragma once
/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	ctcp: Client-To-Client Protocol
*/

#include <stdbool.h>

int ctcp_strip(char *msg, const char *src, int b2, bool ha, bool notice, bool priv, bool tx); // strip out and handle CTCP queries from a message
int ctcp(const char *msg, const char *src, int b2, bool ha, bool notice, bool priv, bool tx); // handle a CTCP query

