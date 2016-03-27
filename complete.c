/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-16 Edward Cree

	See quirc.c for license information
	complete: tab completion
*/

#include "complete.h"
#include "strbuf.h"

size_t find_word(const iline *inp, size_t right)
{
	size_t sp = right;

	if (sp)
		sp--;
	while (sp > 0 && !strchr (" \t", inp->left.data[sp - 1]))
		sp--;
	return sp;
}

name *find_names(const char *word, size_t len, name *list, size_t *mlen_out)
{
	name *curr = list, *found = NULL;
	size_t mlen = 0;

	while (curr)
	{
		if (!irc_strncasecmp(word, curr->data, len, bufs[cbuf].casemapping))
		{
			name *old = found;
			n_add(&found, curr->data, bufs[cbuf].casemapping);
			if (old && (old->data))
			{
				size_t i;
				for (i = 0; i < mlen; i++)
					if (irc_to_upper(curr->data[i], bufs[cbuf].casemapping) !=
					    irc_to_upper(old->data[i],  bufs[cbuf].casemapping))
						break;
				mlen = i;
			}
			else
			{
				mlen = strlen(curr->data);
			}
		}
		curr = curr->next;
	}
	if (mlen_out)
		*mlen_out = mlen;
	return found;
}

/* Add data to inp->left, duplicating any backslashes */
void i_add(iline *inp, size_t n, const char *data)
{
	for (size_t i = 0; i < n; i++)
	{
		if (!data[i])
			break;
		if (data[i] == '\\')
			append_char(&inp->left.data, &inp->left.l,
				    &inp->left.i, data[i]);
		append_char(&inp->left.data, &inp->left.l,
			    &inp->left.i, data[i]);
	}
}

void tab_complete(iline *inp)
{
	size_t right = inp->left.i, left = find_word(inp, right);
	size_t len = right - left, mlen;
	const char *word = inp->left.data + left;
	name *found = find_names(word, len, bufs[cbuf].nlist, &mlen);
	
	if (found)
	{
		size_t count = 0;
		for (name *curr = found; curr; curr = curr->next)
			count++;
		if ((mlen > len) && (count > 1))
		{
			for (size_t i = 0; i < len; i++)
				back_ichar(&inp->left);
			const char *p = found->data;
			i_add(inp, mlen, p);
			ttab = false;
		}
		else if ((count > 16) && !ttab)
		{
			add_to_buffer (cbuf, STA, NORMAL, 0,
				       false,
				       "Multiple matches (over 16; tab again to list)",
				       "[tab] ");
			ttab = true;
		}
		else if (count > 1)
		{
			char *fmsg;
			size_t l, i;
			init_char(&fmsg, &l, &i);
			while (found)
			{
				append_str(&fmsg, &l, &i, found->data);
				found = found->next;
				if (--count)
					append_str(&fmsg, &l, &i, ", ");
			}
			if (!ttab)
				add_to_buffer(cbuf, STA, NORMAL, 0, false,
					      "Multiple matches", "[tab] ");
			add_to_buffer(cbuf, STA, NORMAL, 0, false, fmsg, "[tab] ");
			free(fmsg);
			ttab = false;
		}
		else
		{
			for (size_t i = 0; i < len; i++)
				back_ichar(&inp->left);
			const char *p = found->data;
			i_add(inp, -1, p);
			if (!left)
				append_char(&inp->left.data, &inp->left.l,
					    &inp->left.i, ':');
			append_char(&inp->left.data, &inp->left.l,
				    &inp->left.i, ' ');
			ttab = false;
		}
	}
	else
	{
		add_to_buffer(cbuf, STA, NORMAL, 0, false,
			      "No nicks match", "[tab] ");
	}
	n_free(found);
}
