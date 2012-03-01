/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-11 Edward Cree

	See quirc.c for license information
	colour: defined colours & mirc-colour-compat
*/

#include "colour.h"

int setcolour(colour c)
{
	return(setcol(c.fore, c.back, c.hi, c.ul));
}

int s_setcolour(colour c, char **rv, int *l, int *i)
{
	return(s_setcol(c.fore, c.back, c.hi, c.ul, rv, l, i));
}

colour c_mirc(int fore, int back)
{
	/*mIRC colours:
0=white, 1=black, 2=dk blue, 3=green, 4=red, 5=maroon, 6=purple, 7=orange, 8=yellow, 9=lt green, 10=teal, 11=cyan, 12=blue, 13=fuchsia, 14=dk gray, 15=lt gray
converted:
0->7, 1->0, 2->4, 3->2, 4->1, 5->1, 6->5, 7->3, 8->3, 9->2, 10->6, 11->6, 12->4, 13->5, 14->0(7), 15->7
*/
	const int col[16]={7, 0, 4, 2, 1, 1, 5, 3, 3, 2, 6, 6, 4, 5, 0, 7};
	colour rv={7, 0, false, false};
	if((fore>=0) && (fore<16))
	{
		rv.fore=col[fore];
	}
	if((back>=0) && (back<16))
	{
		rv.back=col[back];
	}
	if(rv.fore==rv.back)
	{
		rv.fore=7-rv.back;
	}
	return(rv);
}

int c_init(void)
{
	#include "c_init.c"
	return(0);
}
