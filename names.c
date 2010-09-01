/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	names: handling for name lists
*/

#include "names.h"

name * n_add(name ** list, char *data)
{
	if(!list)
		return(NULL);
	n_cull(list, data);
	name *new=(name *)malloc(sizeof(name));
	new->data=strdup(data);
	new->icase=false;
	new->pms=false;
	new->prev=NULL;
	new->next=*list;
	if(*list)
		(*list)->prev=new;
	*list=new;
	return(new);
}

int n_cull(name ** list, char *data)
{
	if(!list)
		return(0);
	int rv=0;
	name *curr=*list;
	while(curr)
	{
		name *next=curr->next;
		if(strcmp(curr->data, data)==0)
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
		if(list->data)
			free(list->data);
		free(list);
	}
}

int i_match(name * list, char *nm, bool pm)
{
	int rv=0;
	name *curr=list;
	while(curr)
	{
		if((!pm)||(curr->pms))
		{
			regex_t comp;
			if(regcomp(&comp, curr->data, REG_EXTENDED|REG_NOSUB|(curr->icase?REG_ICASE:0))==0)
			{
				if(regexec(&comp, nm, 0, NULL, 0)==0)
				{
					rv++;
				}
				regfree(&comp);
			}
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
	char rm[strlen(nm)+2];
	if(strchr(nm, '@'))
		strcpy(rm, nm);
	else
		sprintf(rm, "%s@", nm);
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

void i_list(int b)
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
			w_buf_print(cbuf, c_status, curr->data, tag);
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
			w_buf_print(cbuf, c_status, curr->data, tag);
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
		w_buf_print(cbuf, c_status, curr->data, tag);
		count++;
		curr=next;
	}
	if(!count)
	{
		w_buf_print(cbuf, c_status, "No active ignores for this view.", "/ignore -l: ");
	}
}
