#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	osconf.h: platform-specific configuration settings
*/

#ifndef ASYNCH_NL
	#define ASYNCH_NL	1 // set to 0 if platform does not provide getaddrinfo_a() (and remove -lanl from Makefile OPTLIBS)
#endif /* !ASYNCH_NL */

#ifndef INTMAX_BUG
	#define INTMAX_BUG	0 // set to 1 if platform doesn't like printf("...%jd...", (intmax_t)x); e.g. musl libc
#endif

#if INTMAX_BUG
	#define PRINTMAX	"%lld"
	#define TYPEINTMAX	long long int
#else
	#define PRINTMAX	"%jd"
	#define TYPEINTMAX	intmax_t
#endif
#define CASTINTMAX	(TYPEINTMAX)
