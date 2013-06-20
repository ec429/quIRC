/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	process: subprocess & symbiont control
*/

#include "process.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "buffer.h"
#include "strbuf.h"
#include "bits.h"

bufspec parse_bufspec(const char *spec, bufspec home)
{
	bufspec rv;
	size_t l_first=strcspn(spec, " \t");
	char *first=strndup(spec, l_first);
	if(strcmp(first, "status")==0) // we assume the rest-of-line is fine; should perhaps check it?
	{
		free(first);
		rv.type=BS_STATUS;
		rv.server=NULL;
		rv.channel=NULL;
		return(rv);
	}
	if(strcmp(first, "any")==0) // similarly to above
	{
		free(first);
		rv.type=BS_ANY;
		rv.server=NULL;
		rv.channel=NULL;
		return(rv);
	}
	if(strcmp(first, "home")==0) // similarly to above
	{
		free(first);
		return(clone_bufspec(home));
	}
	rv.server=first;
	size_t o_next=l_first+strspn(spec+l_first, " \t");
	if(spec[o_next]=='!')
	{
		rv.type=BS_NICK;
		o_next++;
	}
	else
		rv.type=BS_CHANNEL;
	char *next=strdup(spec+o_next);
	if(strcmp(next, "server")==0)
	{
		free(next);
		rv.type=BS_SERVER;
		rv.channel=NULL;
		return(rv);
	}
	rv.channel=next;
	return(rv);
}

bufspec clone_bufspec(bufspec spec)
{
	bufspec rv=spec;
	if(rv.server) rv.server=strdup(rv.server);
	if(rv.channel) rv.channel=strdup(rv.channel);
	return(rv);
}

char *print_bufspec(bufspec spec)
{
	char *msg; size_t l,i;
	init_char(&msg, &l, &i);
	switch(spec.type)
	{
		case BS_STATUS:
			append_str(&msg, &l, &i, "status 0");
		break;
		case BS_ANY:
			append_str(&msg, &l, &i, "any 0");
		break;
		case BS_SERVER:
			if(!spec.server)
			{
				free(msg);
				return(NULL);
			}
			append_str(&msg, &l, &i, spec.server);
			append_str(&msg, &l, &i, " server");
		break;
		case BS_CHANNEL:
		case BS_NICK:
			if(!(spec.channel&&spec.server))
			{
				free(msg);
				return(NULL);
			}
			append_str(&msg, &l, &i, spec.server);
			append_str(&msg, &l, &i, spec.type==BS_CHANNEL?" ":" !");
			append_str(&msg, &l, &i, spec.channel);
		break;
		default:
			free(msg);
			// shouldn't get here!
			char emsg[80];
			snprintf(emsg, 80, "print_bufspec didn't recognise type %d", spec.type);
			add_to_buffer(0, MT_ERR, PRIO_QUIET, 0, false, emsg, "Internal error: ");
			return(NULL);
	}
	return(msg);
}

int resolve_bufspec(bufspec spec)
{
	switch(spec.type)
	{
		case BS_STATUS:
			return(0); // status is always buf 0
		case BS_ANY:
			return(-1); // 'any' isn't valid except as a matcher
		case BS_SERVER:
			if(!spec.server) return(-1);
			for(int b=1;b<nbufs;b++)
			{
				if(bufs[b].type!=BT_SERVER) continue;
				if(strcmp(spec.server, bufs[b].realsname)==0)
					return(b);
			}
			return(-1); // not found
		case BS_CHANNEL:
		case BS_NICK:
			if(!spec.channel) return(-1);
			for(int b2=1;b2<nbufs;b2++)
			{
				if(bufs[b2].type!=(spec.type==BS_CHANNEL?BT_CHANNEL:BT_PRIVATE)) continue;
				if(spec.server) // it _should_ always be specified, but we'll be lax and allow NULL => wildcard
				{
					int b=bufs[b2].server;
					if(bufs[b].type!=BT_SERVER) continue; // shouldn't ever happen but let's check
					if(strcmp(spec.server, bufs[b].realsname)!=0) continue;
				}
				if(strcmp(spec.channel, bufs[b2].bname)==0)
					return(b2);
			}
			return(-1); // not found
	}
	// shouldn't get here!
	char msg[80];
	snprintf(msg, 80, "resolve_bufspec didn't recognise type %d", spec.type);
	add_to_buffer(0, MT_ERR, PRIO_QUIET, 0, false, msg, "Internal error: ");
	return(-1);
}

void free_bufspec(bufspec spec)
{
	free(spec.server);
	free(spec.channel);
}

int fork_symbiont(symbiont *buf, char *const *argvl)
{
	if(!buf)
	{
		add_to_buffer(0, MT_ERR, PRIO_QUIET, 0, false, "fork_symbiont called with buf=NULL", "Internal error: ");
		return(1);
	}
	int hbuf=resolve_bufspec(buf->home);
	if(hbuf<0)
	{
		char *msg; size_t l,i;
		init_char(&msg, &l, &i);
		append_str(&msg, &l, &i, "fork_symbiont called with bad home buffer '");
		char *name=print_bufspec(buf->home);
		append_str(&msg, &l, &i, name);
		free(name);
		append_char(&msg, &l, &i, '\'');
		add_to_buffer(0, MT_ERR, PRIO_QUIET, 0, false, msg, "Internal error: ");
		return(1);
	}
	int pipes[3][2];
	if(pipe2(pipes[0], O_NONBLOCK|O_CLOEXEC))
	{
		errno_print(hbuf, "fork_symbiont: pipe2");
		return(1);
	}
	if(pipe2(pipes[1], O_NONBLOCK|O_CLOEXEC))
	{
		close(pipes[0][0]);
		close(pipes[0][1]);
		errno_print(hbuf, "fork_symbiont: pipe2");
		return(1);
	}
	if(pipe2(pipes[2], O_NONBLOCK|O_CLOEXEC))
	{
		close(pipes[0][0]);
		close(pipes[0][1]);
		close(pipes[1][0]);
		close(pipes[1][1]);
		errno_print(hbuf, "fork_symbiont: pipe2");
		return(1);
	}
	buf->pid=fork();
	if(buf->pid<0)
	{
		close(pipes[0][0]);
		close(pipes[0][1]);
		close(pipes[1][0]);
		close(pipes[1][1]);
		close(pipes[2][0]);
		close(pipes[2][1]);
		errno_print(hbuf, "fork_symbiont: fork");
		return(1);
	}
	if(buf->pid) // parent
	{
		close(pipes[0][0]); // don't need to read stdin
		close(pipes[1][1]); // or write stdout
		close(pipes[2][1]); // or stderr
		buf->in=pipes[0][1]; // but we do write stdin
		buf->out=pipes[1][0]; // read stdout
		buf->err=pipes[2][0]; // and stderr
		buf->rx=RXM_nil;
		buf->grab=NULL;
		return(0);
	}
	// child
	if(dup2(pipes[0][0], STDIN_FILENO)<0)
	{
		const char *msg="fork_symbiont: dup2(stdin): ";
		int e=errno;
		write(pipes[2][1], msg, strlen(msg));
		msg=strerror(e);
		write(pipes[2][1], msg, strlen(msg));
		exit(EXIT_FAILURE);
	}
	if(dup2(pipes[1][1], STDOUT_FILENO)<0)
	{
		const char *msg="fork_symbiont: dup2(stdout): ";
		int e=errno;
		write(pipes[2][1], msg, strlen(msg));
		msg=strerror(e);
		write(pipes[2][1], msg, strlen(msg));
		exit(EXIT_FAILURE);
	}
	if(dup2(pipes[2][1], STDERR_FILENO)<0)
	{
		const char *msg="fork_symbiont: dup2(stderr): ";
		int e=errno;
		write(pipes[2][1], msg, strlen(msg));
		msg=strerror(e);
		write(pipes[2][1], msg, strlen(msg));
		exit(EXIT_FAILURE);
	}
	execv(argvl[0], argvl);
	// if we're still here, it must have failed
	{
		const char *msg="fork_symbiont: execv: ";
		int e=errno;
		write(pipes[2][1], msg, strlen(msg));
		msg=strerror(e);
		write(pipes[2][1], msg, strlen(msg));
		exit(EXIT_FAILURE);
	}
}
