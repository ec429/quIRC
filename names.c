/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-12 Edward Cree

	See quirc.c for license information
	names: handling for name lists
*/

#include "names.h"

name * n_add(name ** list, char *data, cmap casemapping)
{
	if(!list)
		return(NULL);
	n_cull(list, data, casemapping);
	name *ptr=*list;
	bool last=false;
	while(ptr&&(irc_strcasecmp(data, ptr->data, casemapping)>0))
	{
		if(ptr->next)
		{
			ptr=ptr->next;
		}
		else
		{
			last=true;
			break;
		}
	}
	name *new=(name *)malloc(sizeof(name));
	new->data=strdup(data);
	new->icase=false;
	new->pms=false;
	new->npfx=0;
	new->prefixes=NULL;
	if(ptr)
	{
		if(last)
		{
			new->prev=ptr;
			new->next=NULL;
			ptr->next=new;
		}
		else
		{
			if(ptr->prev)
			{
				ptr->prev->next=new;
				new->prev=ptr->prev;
			}
			else
			{
				*list=new;
				new->prev=NULL;
			}
			ptr->prev=new;
			new->next=ptr;
		}
	}
	else
	{
		new->prev=NULL;
		new->next=*list;
		if(*list)
			(*list)->prev=new;
		*list=new;
	}
	return(new);
}

int n_cull(name ** list, char *data, cmap casemapping)
{
	if(!list)
		return(0);
	int rv=0;
	name *curr=*list;
	while(curr)
	{
		name *next=curr->next;
		if(irc_strcasecmp(curr->data, data, casemapping)==0)
		{
			if(curr->prev)
			{
				curr->prev->next=curr->next;
			}
			else
			{
				*list=curr->next;
			}
			if(curr->next)
				curr->next->prev=curr->prev;
			free(curr->data);
			free(curr);
			rv++;
		}
		curr=next;
	}
	return(rv);
}

void n_free(name * list)
{
	if(list)
	{
		n_free(list->next);
		free(list->data);
		free(list->prefixes);
		free(list);
	}
}

int i_match(name * list, char *nm, bool pm, cmap casemapping)
{
	int rv=0;
	name *curr=list;
	while(curr)
	{
		if((!pm)||(curr->pms))
		{
			char *data;
			if(curr->icase)
			{
				int l,i;
				init_char(&data, &l, &i);
				char *p=curr->data;
				while(*p)
				{
					append_char(&data, &l, &i, '[');
					if(irc_to_lower(*p, casemapping)=='^')
						append_char(&data, &l, &i, '\\');
					if(irc_to_lower(*p, casemapping)==']')
						append_char(&data, &l, &i, '\\');
					if(irc_to_lower(*p, casemapping)=='\\')
						append_char(&data, &l, &i, '\\');
					append_char(&data, &l, &i, irc_to_lower(*p, casemapping)); // because of irc casemapping weirdness, we have to do our REG_ICASE by hand
					if(irc_to_upper(*p, casemapping)==']')
						append_char(&data, &l, &i, '\\');
					if(irc_to_upper(*p, casemapping)=='\\')
						append_char(&data, &l, &i, '\\');
					append_char(&data, &l, &i, irc_to_upper(*p, casemapping));
					append_char(&data, &l, &i, ']');
				}
			}
			else
			{
				data=strdup(curr->data);
			}
			regex_t comp;
			if(regcomp(&comp, data, REG_EXTENDED|REG_NOSUB)==0)
			{
				if(regexec(&comp, nm, 0, NULL, 0)==0)
				{
					rv++;
				}
				regfree(&comp);
			}
			free(data);
		}
		curr=curr->next;
	}
	return(rv);
}

int i_cull(name ** list, char *nm)
{
	if(!list)
		return(0);
	int rv=0;
	char *pnm=strdup(nm?nm:"");
	char *src, *user, *host;
	prefix_split(pnm, &src, &user, &host);
	char rm[strlen(src)+strlen(user)+strlen(host)+3];
	sprintf(rm, "%s!%s@%s", src, user, host);
	name *curr=*list;
	while(curr)
	{
		name *next=curr->next;
		regex_t comp;
		if(regcomp(&comp, curr->data, REG_EXTENDED|REG_NOSUB|(curr->icase?REG_ICASE:0))==0)
		{
			if(regexec(&comp, rm, 0, NULL, 0)==0)
			{
				if(curr->prev)
				{
					curr->prev->next=curr->next;
				}
				else
				{
					*list=curr->next;
				}
				if(curr->next)
					curr->next->prev=curr->prev;
				free(curr->data);
				free(curr);
				rv++;
			}
			regfree(&comp);
		}
		curr=next;
	}
	return(rv);
}

void i_list(void)
{
	int count=0;
	if(bufs[cbuf].type==CHANNEL)
	{
		name *curr=bufs[cbuf].ilist;
		while(curr)
		{
			name *next=curr->next;
			char tag[20];
			sprintf(tag, "/ignore -l: C%s%s\t", curr->pms?"p":"", curr->icase?"i":"");
			add_to_buffer(cbuf, STATUS, NORMAL, 0, false, curr->data, tag);
			count++;
			curr=next;
		}
	}
	if(bufs[cbuf].server)
	{
		name *curr=bufs[bufs[cbuf].server].ilist;
		while(curr)
		{
			name *next=curr->next;
			char tag[20];
			sprintf(tag, "/ignore -l: S%s%s\t", curr->pms?"p":"", curr->icase?"i":"");
			add_to_buffer(cbuf, STATUS, NORMAL, 0, false, curr->data, tag);
			count++;
			curr=next;
		}
	}
	name *curr=bufs[0].ilist;
	while(curr)
	{
		name *next=curr->next;
		char tag[20];
		sprintf(tag, "/ignore -l: *%s%s\t", curr->pms?"p":"", curr->icase?"i":"");
		add_to_buffer(cbuf, STATUS, NORMAL, 0, false, curr->data, tag);
		count++;
		curr=next;
	}
	if(!count)
	{
		add_to_buffer(cbuf, STATUS, NORMAL, 0, false, "No active ignores for this view.", "/ignore -l: ");
	}
}
