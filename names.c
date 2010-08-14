/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	names: handling for name lists
*/

#include "names.h"

name * n_add(name ** list, char *data)
{
	n_cull(list, data);
	name *new=(name *)malloc(sizeof(name));
	new->data=strdup(data);
	new->prev=NULL;
	new->next=*list;
	if(*list)
		(*list)->prev=new;
	*list=new;
	return(new);
}

int n_cull(name ** list, char *data)
{
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
