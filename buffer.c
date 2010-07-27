/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	buffer: multiple-buffer control
*/

int init_buffer(buffer *buf, btype type, char *bname, int nlines)
{
	buffer[0].type=type;
	buffer[0].bname=bname;
	buffer[0].nlines=nlines;
	buffer[0].ptr=0;
	buffer[0].lc=(colour *)malloc(nlines*sizeof(colour));
	buffer[0].lt=(char **)malloc(nlines*sizeof(char *));
	buffer[0].ts=(time_t *)malloc(nlines*sizeof(time_t));
	return(0);
}

int add_to_buffer(buffer *buf, colour lc, char *lt, time_t ts)
{
	
}
