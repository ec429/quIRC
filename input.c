/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	input: handle input routines
*/

#include "input.h"
#include "logging.h"
#include "strbuf.h"
#include "ttyesc.h"
#include "buffer.h"
#include "irc.h"
#include "bits.h"
#include "config.h"
#include "keymod.h"
#include "cmd.h"
#include "complete.h"

size_t i_firstlen (ichar src);
size_t i_lastlen (ichar src);
void i_move (iline * inp, ssize_t bytes);

void inputchar (iline * inp, int *state)
{
	int c = getchar ();
	if ((c == 0) || (c == EOF))	// stdin is set to non-blocking, so this may happen
		return;
	char *seq;
	size_t sl, si;
	int mod = -1;
	init_char (&seq, &sl, &si);
	bool match = true;
	while (match && (mod < 0))
	{
		append_char (&seq, &sl, &si, c);
		match = false;
		for (unsigned int i = 0; i < nkeys; i++)
		{
			if (strncmp (seq, kmap[i].mod, si) == 0)
			{
				if (kmap[i].mod[si] == 0)
				{
					mod = i;
					break;
				}
				else
				{
					match = true;
				}
			}
		}
		if (match && (mod < 0))
		{
			c = getchar ();
			if ((c == 0) || (c == EOF))
			{
				match = false;
			}
		}
	}
	if (mod < 0)
	{
		while (si > 1)
			ungetc (seq[--si], stdin);
		append_char (&inp->left.data, &inp->left.l, &inp->left.i,
			     seq[0]);
	}
	free (seq);
	if (c != '\t')
		ttab = false;
	if (mod == KEY_BS)	// backspace
	{
		size_t ll = i_lastlen (inp->left);
		for (size_t i = 0; i < ll; i++)
			back_ichar (&inp->left);
		return;
	}
	if ((mod < 0) && (c < 32))	// this also stomps on the newline
	{
		back_ichar (&inp->left);
		if (c == 8)	// C-h ~= backspace
		{
			size_t ll = i_lastlen (inp->left);
			for (size_t i = 0; i < ll; i++)
				back_ichar (&inp->left);
			return;
		}
		if (c == 1)	// C-a ~= home
		{
			i_home (inp);
			return;
		}
		if (c == 5)	// C-e ~= end
		{
			i_end (inp);
			return;
		}
		if (c == 3)	// C-c ~= clear
		{
			ifree (inp);
			return;
		}
		if (c == 24)	// C-x ~= clear to left
		{
			free (inp->left.data);
			inp->left.data = NULL;
			inp->left.i = inp->left.l = 0;
			return;
		}
		if (c == 11)	// C-k ~= clear to right
		{
			free (inp->right.data);
			inp->right.data = NULL;
			inp->right.i = inp->right.l = 0;
			return;
		}
		if (c == 23)	// C-w ~= backspace word
		{
			while (back_ichar (&inp->left) == ' ');
			while (!strchr (" ", back_ichar (&inp->left)));
			if (inp->left.i)
				append_char (&inp->left.data, &inp->left.l,
					     &inp->left.i, ' ');
			return;
		}
		if (c == '\t')	// tab completion of nicks
		{
			tab_complete(inp);
			return;
		}
	}
	else if (mod >= 0)
	{
		bool gone = false;
		for (int n = 1; n <= 12; n++)
		{
			if (mod == KEY_F (n))
			{
				cbuf = min (n % 12, nbufs - 1);
				redraw_buffer ();
				return;
			}
		}
		if (mod == KEY_UP)	// Up
		{
			int old = bufs[cbuf].input.scroll;
			bufs[cbuf].input.scroll =
				min (bufs[cbuf].input.scroll + 1,
				     bufs[cbuf].input.filled ? bufs[cbuf].
				     input.nlines - 1 : bufs[cbuf].input.ptr);
			if (old != bufs[cbuf].input.scroll)
				gone = true;
			if (gone && !old)
			{
				if (inp->left.i || inp->right.i)
				{
					char out[inp->left.i + inp->right.i +
						 1];
					sprintf (out, "%s%s",
						 inp->left.data ? inp->left.
						 data : "",
						 inp->right.data ? inp->right.
						 data : "");
					addtoibuf (&bufs[cbuf].input, out);
					bufs[cbuf].input.scroll = 2;
				}
			}
		}
		if (mod == KEY_DOWN)	// Down
		{
			gone = true;
			if (!bufs[cbuf].input.scroll)
			{
				if (inp->left.i || inp->right.i)
				{
					char out[inp->left.i + inp->right.i +
						 1];
					sprintf (out, "%s%s",
						 inp->left.data ? inp->left.
						 data : "",
						 inp->right.data ? inp->right.
						 data : "");
					addtoibuf (&bufs[cbuf].input, out);
					bufs[cbuf].input.scroll = 0;
				}
			}
			bufs[cbuf].input.scroll =
				max (bufs[cbuf].input.scroll - 1, 0);
		}
		if (gone && (bufs[cbuf].input.ptr || bufs[cbuf].input.filled))
		{
			if (bufs[cbuf].input.scroll)
			{
				char *ln =
					bufs[cbuf].input.
					line[(bufs[cbuf].input.ptr +
					      bufs[cbuf].input.nlines -
					      bufs[cbuf].input.scroll) %
					     bufs[cbuf].input.nlines];
				if (ln)
				{
					ifree (inp);
					inp->left.data = strdup (ln);
					inp->left.i = strlen (inp->left.data);
					inp->left.l = 0;
				}
			}
			else
			{
				ifree (inp);
			}
			return;
		}
		if (mod == KEY_RIGHT)
		{
			i_move (inp, i_firstlen (inp->right));
			return;
		}
		if (mod == KEY_LEFT)
		{
			i_move (inp, -i_lastlen (inp->left));
			return;
		}
		if (mod == KEY_HOME)
		{
			i_home (inp);
			return;
		}
		if (mod == KEY_END)
		{
			i_end (inp);
			return;
		}
		if (mod == KEY_DELETE)
		{
			size_t fl = i_firstlen (inp->right);
			if (inp->right.data && (inp->right.i > fl))
			{
				char *nr = strdup (inp->right.data + fl);
				free (inp->right.data);
				inp->right.data = nr;
				inp->right.i -= fl;
				inp->right.l = inp->right.i;
			}
			else
			{
				free (inp->right.data);
				inp->right.data = NULL;
				inp->right.l = inp->right.i = 0;
			}
			return;
		}
		if (mod == KEY_CPGUP)
		{
			bufs[cbuf].ascroll -= height - (tsb ? 3 : 2);
			redraw_buffer ();
			return;
		}
		if (mod == KEY_CPGDN)
		{
			bufs[cbuf].ascroll += height - (tsb ? 3 : 2);
			redraw_buffer ();
			return;
		}
		gone = false;
		if (mod == KEY_PGUP)
		{
			int old = bufs[cbuf].input.scroll;
			bufs[cbuf].input.scroll =
				min (bufs[cbuf].input.scroll + 10,
				     bufs[cbuf].input.filled ? bufs[cbuf].
				     input.nlines - 1 : bufs[cbuf].input.ptr);
			if (old != bufs[cbuf].input.scroll)
				gone = true;
			if (gone && !old)
			{
				if (inp->left.i || inp->right.i)
				{
					char out[inp->left.i + inp->right.i +
						 1];
					sprintf (out, "%s%s",
						 inp->left.data ? inp->left.
						 data : "",
						 inp->right.data ? inp->right.
						 data : "");
					addtoibuf (&bufs[cbuf].input, out);
					bufs[cbuf].input.scroll = 2;
				}
			}
			return;
		}
		if (mod == KEY_PGDN)
		{
			gone = true;
			if (!bufs[cbuf].input.scroll)
			{
				if (inp->left.i || inp->right.i)
				{
					char out[inp->left.i + inp->right.i +
						 1];
					sprintf (out, "%s%s",
						 inp->left.data ? inp->left.
						 data : "",
						 inp->right.data ? inp->right.
						 data : "");
					addtoibuf (&bufs[cbuf].input, out);
					bufs[cbuf].input.scroll = 0;
				}
			}
			bufs[cbuf].input.scroll =
				max (bufs[cbuf].input.scroll - 10, 0);
			return;
		}
		if (gone && (bufs[cbuf].input.ptr || bufs[cbuf].input.filled))
		{
			if (bufs[cbuf].input.scroll)
			{
				char *ln =
					bufs[cbuf].input.
					line[(bufs[cbuf].input.ptr +
					      bufs[cbuf].input.nlines -
					      bufs[cbuf].input.scroll) %
					     bufs[cbuf].input.nlines];
				if (ln)
				{
					ifree (inp);
					inp->left.data = strdup (ln);
					inp->left.i = strlen (inp->left.data);
					inp->left.l = 0;
				}
			}
			else
			{
				ifree (inp);
			}
			return;
		}
		if ((mod == KEY_SLEFT) || (mod == KEY_CLEFT)
		    || (mod == KEY_ALEFT))
		{
			cbuf = max (cbuf - 1, 0);
			redraw_buffer ();
			return;
		}
		if ((mod == KEY_SRIGHT) || (mod == KEY_CRIGHT)
		    || (mod == KEY_ARIGHT))
		{
			cbuf = min (cbuf + 1, nbufs - 1);
			redraw_buffer ();
			return;
		}
		if ((mod == KEY_CUP) || (mod == KEY_AUP))
		{
			bufs[cbuf].ascroll--;
			redraw_buffer ();
			return;
		}
		if ((mod == KEY_CDOWN) || (mod == KEY_ADOWN))
		{
			bufs[cbuf].ascroll++;
			redraw_buffer ();
			return;
		}
		if ((mod == KEY_SHOME) || (mod == KEY_CHOME)
		    || (mod == KEY_AHOME))
		{
			bufs[cbuf].scroll =
				bufs[cbuf].filled ? (bufs[cbuf].ptr +
						     1) %
				bufs[cbuf].nlines : 0;
			bufs[cbuf].ascroll = 0;
			redraw_buffer ();
			return;
		}
		if ((mod == KEY_SEND) || (mod == KEY_CEND)
		    || (mod == KEY_AEND))
		{
			bufs[cbuf].scroll = bufs[cbuf].ptr;
			bufs[cbuf].ascroll = 0;
			redraw_buffer ();
			return;
		}
	}
	if ((c & 0xe0) == 0xc0)	// 110xxxxx -> 2 bytes of UTF-8
	{
		int d = getchar ();
		if (d && (d != EOF) && ((d & 0xc0) == 0x80))	// 10xxxxxx - UTF middlebyte
			append_char (&inp->left.data, &inp->left.l,
				     &inp->left.i, d);
		else
			ungetc (d, stdin);
		return;
	}
	if ((c & 0xf0) == 0xe0)	// 1110xxxx -> 3 bytes of UTF-8
	{
		int d = getchar ();
		if (d && (d != EOF) && ((d & 0xc0) == 0x80))	// 10xxxxxx - UTF middlebyte
		{
			int e = getchar ();
			if (e && (e != EOF) && ((e & 0xc0) == 0x80))	// 10xxxxxx - UTF middlebyte
			{
				append_char (&inp->left.data, &inp->left.l,
					     &inp->left.i, d);
				append_char (&inp->left.data, &inp->left.l,
					     &inp->left.i, e);
			}
			else
			{
				ungetc (e, stdin);
				ungetc (d, stdin);
			}
		}
		else
			ungetc (d, stdin);
		return;
	}
	if ((c & 0xf8) == 0xf0)	// 11110xxx -> 4 bytes of UTF-8
	{
		int d = getchar ();
		if (d && (d != EOF) && ((d & 0xc0) == 0x80))	// 10xxxxxx - UTF middlebyte
		{
			int e = getchar ();
			if (e && (e != EOF) && ((e & 0xc0) == 0x80))	// 10xxxxxx - UTF middlebyte
			{
				int f = getchar ();
				if (f && (f != EOF) && ((f & 0xc0) == 0x80))	// 10xxxxxx - UTF middlebyte
				{
					append_char (&inp->left.data,
						     &inp->left.l,
						     &inp->left.i, d);
					append_char (&inp->left.data,
						     &inp->left.l,
						     &inp->left.i, e);
					append_char (&inp->left.data,
						     &inp->left.l,
						     &inp->left.i, f);
				}
				else
				{
					ungetc (f, stdin);
					ungetc (e, stdin);
					ungetc (d, stdin);
				}
			}
			else
			{
				ungetc (e, stdin);
				ungetc (d, stdin);
			}
		}
		else
			ungetc (d, stdin);
		return;
	}
	if (c == '\n')
	{
		*state = 3;
		char out[inp->left.i + inp->right.i + 1];
		sprintf (out, "%s%s", inp->left.data ? inp->left.data : "",
			 inp->right.data ? inp->right.data : "");
		addtoibuf (&bufs[cbuf].input, out);
		ifree (inp);
		return;
	}
	return;
}

char *slash_dequote (char *inp)
{
	size_t l = strlen (inp) + 1;
	char *rv = (char *) malloc (l + 1);	// we only get shorter, so this will be enough
	unsigned int o = 0;
	while ((*inp) && (o < l))	// o>=l should never happen, but it's covered just in case
	{
		if (*inp == '\\')	// \n, \r, \\, \ooo (\0 remains escaped)
		{
			char c = *++inp;
			switch (c)
			{
			case 'n':
				rv[o++] = '\n';
				inp++;
				break;
			case 'r':
				rv[o++] = '\r';
				inp++;
				break;
			case '\\':
				rv[o++] = '\\';
				inp++;
				break;
			case '0':	// \000 to \377 are octal escapes
			case '1':
			case '2':
			case '3':
				{
					int digits = 0;
					int oval = c - '0';	// Octal VALue
					while (isdigit (inp[1])
					       && (inp[1] < '8')
					       && (++digits < 3))
					{
						oval *= 8;
						oval += (*++inp) - '0';
					}
					if (oval)
					{
						rv[o++] = oval;
					}
					else	// \0 is a special case (it remains escaped)
					{
						rv[o++] = '\\';
						if (o < l)
							rv[o++] = '0';
					}
					inp++;
				}
				break;
			default:
				rv[o++] = '\\';
				break;
			}
		}
		else
		{
			rv[o++] = *inp++;
		}
	}
	rv[o] = 0;
	return (rv);
}

int cmd_handle (char *inp, char **qmsg, fd_set * master, int *fdmax)	// old state=3; return new state
{
	char *cmd = inp + 1;
	if (*cmd == '/')	//msg sends /msg
		return (talk (cmd));
	char *args = strchr (cmd, ' ');
	if (args)
		*args++ = 0;
	return call_cmd (cmd, args, qmsg, master, fdmax);
}

void initibuf (ibuffer * i)
{
	i->nlines = buflines;
	i->ptr = 0;
	i->scroll = 0;
	i->filled = false;
	i->line = (char **) malloc (i->nlines * sizeof (char *));
}

void addtoibuf (ibuffer * i, char *data)
{
	if (i)
	{
		if (i->filled)
		{
			if (i->line[i->ptr])
				free (i->line[i->ptr]);
		}
		i->line[i->ptr++] = strdup (data ? data : "");
		if (i->ptr >= i->nlines)
		{
			i->ptr = 0;
			i->filled = true;
		}
		i->scroll = 0;
	}
}

void freeibuf (ibuffer * i)
{
	if (i->line)
	{
		int l;
		for (l = 0; l < (i->filled ? i->nlines : i->ptr); l++)
		{
			if (i->line[l])
				free (i->line[l]);
		}
		free (i->line);
	}
}

char back_ichar (ichar * buf)
{
	char c = 0;
	if (buf->i)
	{
		c = buf->data[--(buf->i)];
		buf->data[(buf->i)] = 0;
	}
	return (c);
}

char front_ichar (ichar * buf)
{
	char c = 0;
	if (buf->i)
	{
		c = buf->data[0];
		memmove (buf->data, buf->data + 1, buf->i--);
	}
	return (c);
}

size_t i_firstlen (ichar src)
{
	if (!src.i)
		return (0);
	size_t u;
	if (isutf8 (src.data, &u))
		return (u);
	return (1);
}

size_t i_lastlen (ichar src)
{
	size_t start = max (src.i, 4) - 4, prev = start;
	size_t u;
	while (start < src.i)
	{
		prev = start;
		if (isutf8 (src.data + start, &u))
			start += u;
		else if (src.data[start] & 0x80)
			start++;
		else
		{
			start++;
			if (start + 1 >= src.i)
				break;
		}
	}
	return (start - prev);
}

void i_move (iline * inp, ssize_t bytes)
{
	bool fw = (bytes > 0);
	size_t b = fw ? bytes : -bytes;	// can't use abs() because we don't know what length a size_t is (do we need labs()? llabs()?)
	char c;
	for (size_t i = 0; i < b; i++)
		if (fw)
		{
			if ((c = front_ichar (&inp->right)))
				append_char (&inp->left.data, &inp->left.l,
					     &inp->left.i, c);
		}
		else if ((c = back_ichar (&inp->left)))
			prepend_char (&inp->right.data, &inp->right.l,
				      &inp->right.i, c);
}

void ifree (iline * buf)
{
	free (buf->left.data);
	free (buf->right.data);
	buf->left.data = NULL;
	buf->right.data = NULL;
	buf->left.i = buf->left.l = 0;
	buf->right.i = buf->right.l = 0;
}

void i_home (iline * inp)
{
	if (inp->left.i)
	{
		size_t b = inp->left.i + inp->right.i;
		char *nr = (char *) malloc (b + 1);
		sprintf (nr, "%s%s", inp->left.data ? inp->left.data : "",
			 inp->right.data ? inp->right.data : "");
		ifree (inp);
		inp->right.data = nr;
		inp->right.i = b;
		inp->right.l = b + 1;
	}
}

void i_end (iline * inp)
{
	if (inp->right.i)
	{
		size_t b = inp->left.i + inp->right.i;
		char *nl = (char *) malloc (b + 1);
		sprintf (nl, "%s%s", inp->left.data ? inp->left.data : "",
			 inp->right.data ? inp->right.data : "");
		ifree (inp);
		inp->left.data = nl;
		inp->left.i = b;
		inp->left.l = b + 1;
	}
}
