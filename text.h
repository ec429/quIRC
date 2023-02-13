#pragma once
/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	text: misc. text snippets
*/

// interface text
#define GPL_TAIL	"Copyright (C) 2010-13 Edward Cree\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it under certain conditions.  (GNU GPL v3+)\nFor further details, see the file 'COPYING' in the quirc directory."

#define VERSION_MSG " %s %hhu.%hhu.%hhu%s%s\n\
 Copyright (C) 2010-13 Edward Cree.\n\
 License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
 This is free software: you are free to change and redistribute it.\n\
 There is NO WARRANTY, to the extent permitted by law.\n\
 Compiler was %s\n\
 osconf.h settings were ASYNCH_NL=%d, INTMAX_BUG=%d\n", "quirc", VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_TXT[0]?"-":"", VERSION_TXT, CC_VERSION, ASYNCH_NL, INTMAX_BUG

#define USAGE_MSG "quirc [--no-server] [--no-chan] [--check] [--server=<server>] [--uname=<uname>]\n    [--fname=<fname>] [--nick=<nick>] [--chan=<chan>] [--port=<port>]\n    [--pass=<pass>] [options]\nquirc {-h|--help|-V|--version}\n"
