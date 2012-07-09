BEGIN {FPAT="[0-9a-g]+"}
{print "\
/*\n\
	quIRC - simple terminal-based IRC client\n\
	Copyright (C) 2010-11 Edward Cree\n\
\n\
	See quirc.c for license information\n\
	version.h: contains version number (generated from git describe)\n\
*/\n\
#pragma once\n\
#define VERSION_MAJ "$1" // Major version\n\
#define VERSION_MIN "$2" // Minor version\n\
#define VERSION_REV "$3" // Revision number\n\
#define VERSION_TXT \""$4"-"$5"\" // Rest of git describe"}
