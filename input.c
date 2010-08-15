/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	input: handle input routines
*/

#include "input.h"

int inputchar(char **inp, int *state)
{
	printf("\010\010\010" CLA);
	int ino=(*inp)?strlen(*inp):0;
	*inp=(char *)realloc(*inp, ino+2);
	unsigned char c=(*inp)[ino]=getchar();
	(*inp)[ino+1]=0;
	if(strchr("\010\177", c)) // various backspace-type characters
	{
		if(ino)
			(*inp)[ino-1]=0;
		(*inp)[ino]=0;
	}
	else if((c<32)||(c>127)) // this also stomps on the newline 
	{
		(*inp)[ino]=0;
		if(c==1)
		{
			free(*inp);
			*inp=NULL;
			ino=0;
		}
		if(ino>0)
		{
			if(c=='\t') // tab completion of nicks
			{
				int sp=ino-1;
				while(sp>0 && !strchr(" \t", (*inp)[sp-1]))
					sp--;
				name *curr=bufs[cbuf].nlist;
				name *found=NULL;bool tmany=false;
				while(curr)
				{
					if(strncasecmp((*inp)+sp, curr->data, ino-sp)==0)
					{
						if(tmany)
						{
							buf_print(cbuf, c_err, curr->data, true);
						}
						else if(found)
						{
							buf_print(cbuf, c_err, "[tab] Multiple nicks match", true);
							buf_print(cbuf, c_err, found->data, true);
							buf_print(cbuf, c_err, curr->data, true);
							found=NULL;tmany=true;
						}
						else
							found=curr;
					}
					if(curr)
						curr=curr->next;
				}
				if(found)
				{
					*inp=(char *)realloc(*inp, sp+strlen(found->data)+4);
					if(sp)
						sprintf((*inp)+sp, "%s", found->data);
					else
						sprintf((*inp)+sp, "%s: ", found->data);
				}
				else if(!tmany)
				{
					buf_print(cbuf, c_err, "[tab] No nicks match", true);
				}
			}
		}
	}
	if(c=='\033') // escape sequence
	{
		if(getchar()=='\133') // 1b 5b
		{
			unsigned char d=getchar();
			switch(d)
			{
				case 'D': // left cursor counts as a backspace
					if(ino)
						(*inp)[ino-1]=0;
				break;
				case '3': // take another
					if(getchar()=='~') // delete
					{
						if(ino)
							(*inp)[ino-1]=0;
					}
				break;
				case '5': // ^[[5
				case '6': // ^[[6
					if(getchar()==';')
					{
						if(getchar()=='5')
						{
							if(getchar()=='~')
							{
								if(d=='5') // C-PgUp
								{
									bufs[cbuf].scroll=min(bufs[cbuf].scroll+height-2, bufs[cbuf].filled?bufs[cbuf].nlines-1:bufs[cbuf].ptr-1);
									redraw_buffer();
								}
								else // d=='6' // C-PgDn
								{
									if(bufs[cbuf].scroll)
									{
										bufs[cbuf].scroll=max(bufs[cbuf].scroll-(height-2), 0);
										redraw_buffer();
									}
								}
							}
						}
					}
				break;
				case '1': // ^[[1
					if(getchar()==';')
					{
						if(getchar()=='5')
						{
							switch(getchar())
							{
								case 'D': // C-left
									cbuf=max(cbuf-1, 0);
									redraw_buffer();
								break;
								case 'C': // C-right
									cbuf=min(cbuf+1, nbufs-1);
									redraw_buffer();
								break;
								case 'A': // C-up
									bufs[cbuf].scroll=min(bufs[cbuf].scroll+1, bufs[cbuf].filled?bufs[cbuf].nlines-1:bufs[cbuf].ptr-1);
									redraw_buffer();
								break;
								case 'B': // C-down
									if(bufs[cbuf].scroll)
									{
										bufs[cbuf].scroll=bufs[cbuf].scroll-1;
										redraw_buffer();
									}
								break;
								case 'F': // C-end
									if(bufs[cbuf].scroll)
									{
										bufs[cbuf].scroll=0;
										redraw_buffer();
									}
								break;
								case 'H': // C-home
									bufs[cbuf].scroll=bufs[cbuf].filled?bufs[cbuf].nlines-1:bufs[cbuf].ptr-1;
									redraw_buffer();
								break;
							}
							
						}
					}
				break;
			}
		}
	}
	else if(c==0xc2) // c2 bN = alt-N (for N in 0...9)
	{
		unsigned char d=getchar();
		if((d&0xf0)==0xb0)
		{
			cbuf=min(max(d&0x0f, 0), nbufs-1);
			redraw_buffer();
		}
	}
	if(c=='\n')
	{
		*state=3;
	}
	else
	{
		in_update(*inp);
	}
	return(0);
}
