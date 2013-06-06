/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	ctcp: Client-To-Client Protocol
*/

#include "ctcp.h"
#include <time.h>
#include <string.h>
#include <errno.h>
#include "types.h"
#include "buffer.h"
#include "config.h"
#include "version.h"

int ctcp_strip(char *msg, const char *src, int b2, bool ha, bool notice, bool priv, bool tx)
{
	int e=0;
	char *in=msg, *out=msg;
	while(*in)
	{
		if(*in==1)
		{
			char *t=strchr(in+1, 1);
			if(t)
			{
				*t=0;
				e|=ctcp(in+1, src, b2, ha, notice, priv, tx);
				in=t+1;
				continue;
			}
		}
		*out++=*in++;
	}
	*out=0;
	return(e);
}

int ctcp_action(__attribute__((unused)) int fd, const char *msg, const char *src, int b2, bool ha, bool tx)
{
	add_to_buffer(b2, ACT, NORMAL, 0, tx, msg+7, src);
	ha=ha||strstr(msg+7, SERVER(b2).nick);
	if(ha)
		bufs[b2].hi_alert=5;
	return(0);
}

int ctcp_notice_generic(const char *msg, const char *src, int b2, bool ha)
{
	add_to_buffer(b2, NOTICE, NORMAL, 0, false, msg, src);
	if(ha)
		bufs[b2].hi_alert=5;
	return(0);
}

int ctcp_finger(int fd, __attribute__((unused)) const char *msg, const char *src, __attribute__((unused)) int b2)
{
	char resp[32+strlen(src)+strlen(fname)];
	sprintf(resp, "NOTICE %s :\001FINGER %s\001", src, fname);
	irc_tx(fd, resp);
	return(0);
}

int ctcp_ping(int fd, const char *msg, const char *src, __attribute__((unused)) int b2)
{
	char resp[16+strlen(src)+strlen(msg)];
	sprintf(resp, "NOTICE %s :\001%s\001", src, msg);
	irc_tx(fd, resp);
	return(0);
}

int ctcp_notice_ping(const char *msg, const char *src, int b2, bool ha)
{
	unsigned int t, u;
	ssize_t n;
	if((msg[4]==' ')&&(sscanf(msg+5, "%u%zn", &t, &n)==1))
	{
		double dt=0;
		if((msg[5+n]==' ')&&(sscanf(msg+6+n, "%u", &u)==1))
		{
			struct timeval tv;
			gettimeofday(&tv, NULL);
			dt=(tv.tv_sec-t)+1e-6*(tv.tv_usec-(suseconds_t)u);
		}
		else
			dt=difftime(time(NULL), t);
		char tm[32];
		snprintf(tm, 32, "%gs", dt);
		add_to_buffer(b2, STA, NORMAL, 0, false, tm, "/ping: ");
	}
	else
	{
		add_to_buffer(b2, NOTICE, NORMAL, 0, false, msg, src);
	}
	if(ha)
		bufs[b2].hi_alert=5;
	return(0);
}

int ctcp_clientinfo(int fd, __attribute__((unused)) const char *msg, const char *src, __attribute__((unused)) int b2)
{
	// XXX this doesn't fully implement the CLIENTINFO "protocol", it just gives a list of recognised top-level commands
	char resp[64+strlen(src)];
	sprintf(resp, "NOTICE %s :\001CLIENTINFO ACTION FINGER PING CLIENTINFO TIME SOURCE VERSION\001", src);
	irc_tx(fd, resp);
	return(0);
}

int ctcp_version(int fd, __attribute__((unused)) const char *msg, const char *src, __attribute__((unused)) int b2)
{
	char resp[32+strlen(src)+strlen(version)+strlen(CC_VERSION)];
	sprintf(resp, "NOTICE %s :\001VERSION %s:%s:%s\001", src, "quIRC", version, CC_VERSION);
	irc_tx(fd, resp);
	return(0);
}

int ctcp_source(int fd, __attribute__((unused)) const char *msg, const char *src, __attribute__((unused)) int b2)
{
	// XXX this response doesn't match the SOURCE "protocol", which expects an FTP address; quIRC is distributed by HTTP, so we just give a URL
	char resp[32+strlen(src)+strlen(CLIENT_SOURCE)];
	sprintf(resp, "NOTICE %s :\001SOURCE %s\001", src, CLIENT_SOURCE);
	irc_tx(fd, resp);
	return(0);
}

int ctcp_time(int fd, __attribute__((unused)) const char *msg, const char *src, int b2)
{
	time_t now=time(NULL);
	struct tm *tm=localtime(&now);
	if(!tm)
	{
		add_to_buffer(b2, ERR, QUIET, 0, false, strerror(errno), "CTCP TIME");
		char resp[48+strlen(src)];
		sprintf(resp, "NOTICE %s :\001ERRMSG TIME :localtime()\001", src);
		irc_tx(fd, resp);
	}
	char datebuf[256];
	size_t datelen=strftime(datebuf, sizeof(datebuf), "%F %a %T %z", tm);
	char resp[32+strlen(src)+datelen];
	sprintf(resp, "NOTICE %s :\001TIME %s\001", src, datebuf);
	irc_tx(fd, resp);
	return(0);
}

int ctcp(const char *msg, const char *src, int b2, bool ha, bool notice, bool priv, bool tx)
{
	int b=bufs[b2].server;
	int fd=bufs[b].handle;
	if(priv&&!tx) b2=makeptab(b, src);
	if((strncmp(msg, "ACTION", 6)==0)&&((msg[6]==' ')||(msg[6]==0))) // allow 'ACTION' or 'ACTION blah'
		return(ctcp_action(fd, msg, src, b2, ha, tx));
	if(tx) return(0);
	struct
	{
		const char *query;
		int (*handler)(int fd, const char *msg, const char *src, int b2);
		int (*noticer)(const char *msg, const char *src, int b2, bool ha);
	}
	queries[]=
	{
		{"FINGER", ctcp_finger, ctcp_notice_generic},
		{"PING", ctcp_ping, ctcp_notice_ping},
		{"CLIENTINFO", ctcp_clientinfo, ctcp_notice_generic},
		{"VERSION", ctcp_version, ctcp_notice_generic},
		{"TIME", ctcp_time, ctcp_notice_generic},
		{"SOURCE", ctcp_source, ctcp_notice_generic},
	};
	for(unsigned int i=0;i<sizeof(queries)/sizeof(*queries);i++)
	{
		if(!strncmp(msg, queries[i].query, strlen(queries[i].query)))
		{
			if(notice)
				return(queries[i].noticer(msg, src, b2, ha));
			else
				return(queries[i].handler(fd, msg, src, b2));
		}
	}
	// no match
	add_to_buffer(b2, NOTICE, NORMAL, 0, false, msg, src);
	if(ha)
		bufs[b2].hi_alert=5;
	int space=strcspn(msg, " ");
	char cmsg[32+space];
	sprintf(cmsg, "Unrecognised CTCP %.*s (ignoring)", space, msg);
	add_to_buffer(b2, UNK_NOTICE, QUIET, 0, false, cmsg, src);
	if(!notice)
	{
		char resp[40+strlen(src)+strlen(msg)];
		sprintf(resp, "NOTICE %s :\001ERRMSG %s :Unknown query\001", src, msg);
		irc_tx(fd, resp);
	}
	if(ha)
		bufs[b2].hi_alert=5;
	return(0);
}
