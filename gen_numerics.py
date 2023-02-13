#!/usr/bin/python
# gen_numerics.py: generate numerics.h

import numerics

print """#pragma once
/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	numeric: IRC numeric replies
*/

/***
	This file is generated by gen_numerics.py from masters in numerics.py.
	Do not make edits directly to this file!  Edit the masters instead.
***/

/*
	A symbolic name defined here does not necessarily imply recognition or decoding of that numeric reply.
	Some numeric replies are non-normative; that is, they are not defined in the original RFC1459 or its superseding RFC2812, but instead are either defined in other, non-normative documents, or are entirely experimental.  These are denoted with an X before the name (of the form RPL_X_BOGOSITY); where a numeric is being identified purely on the basis of usage "in the wild", the symbolic name will be completely arbitrary and may not align with usage elsewhere.
*/

/* Error replies */"""
errs = [n for n in numerics.nums.values() if isinstance(n, numerics.NumericError)]
for e in errs:
	print str(e)

print """
/* Command responses */"""
rpls = [n for n in numerics.nums.values() if isinstance(n, numerics.NumericReply)]
for r in rpls:
	print str(r)
