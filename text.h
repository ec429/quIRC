/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-11 Edward Cree

	See quirc.c for license information
	text: misc. text snippets
*/

// interface text
#define GPL_MSG	"quirc -- "GPL_TAIL
#define GPL_TAIL	"Copyright (C) 2010-11 Edward Cree\n\tThis program comes with ABSOLUTELY NO WARRANTY.\n\tThis is free software, and you are welcome to redistribute it\n\tunder certain conditions.  (GNU GPL v3+)\n\tFor further details, see the file 'COPYING' in the quirc directory."

#define VERSION_MSG " %s %hhu.%hhu.%hhu%s%s\n\
 Copyright (C) 2010-11 Edward Cree.\n\
 License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
 This is free software: you are free to change and redistribute it.\n\
 There is NO WARRANTY, to the extent permitted by law.\n\
 Compiler was %s\n", "quirc", VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_TXT[0]?"-":"", VERSION_TXT, CC_VERSION

#define USAGE_MSG "quirc [--width=<cols>] [--height=<rows>] [--maxnicklen=<mnln>] [--mcc=<mcc>]\n\t[--force-redraw=<fred>] [--no-server] [--no-chan] [--check]\n\t[--server=<server>] [--uname=<uname>] [--fname=<fname>] [--nick=<nick>]\n\t[--chan=<chan>] [--port=<port>] [--[no-]fwc] [--[no-]hts] [--[no-]tsb]\nquirc {-h|--help|-V|--version}\n"
