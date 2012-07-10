#pragma once

/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	osconf.h: platform-specific configuration settings
*/

#ifndef ASYNCH_NL
	#define ASYNCH_NL	1 // set to 0 if platform does not provide getaddrinfo_a() (and remove -lanl from Makefile OPTLIBS)
#endif /* ¬ASYNCH_NL */
