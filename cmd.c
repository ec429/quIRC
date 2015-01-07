
#include "buffer.h"
#include "cmd.h"
#include "logging.h"
#include <assert.h>

//Function declarations
CMD_FUN(quit);
CMD_FUN(close);
CMD_FUN(log);
CMD_FUN(set);
CMD_FUN(server);
CMD_FUN(reconnect);
CMD_FUN(disconnect);
CMD_FUN(realsname);
CMD_FUN(join);
CMD_FUN(rejoin);
CMD_FUN(part);
CMD_FUN(unaway);
CMD_FUN(away);
CMD_FUN(afk);
CMD_FUN(nick);
CMD_FUN(topic);
CMD_FUN(msg);
CMD_FUN(ping);
CMD_FUN(amsg);
CMD_FUN(me);
CMD_FUN(tab);
CMD_FUN(sort);
CMD_FUN(left);
CMD_FUN(right);
CMD_FUN(ignore);
CMD_FUN(mode);
CMD_FUN(cmd);

//Number of Commands
#define NCMDS 31

//cmd_funcs can't be malloc'd the regular way because of -Werror
//malloc returns a void pointer and with -Werror you can't
//cast from a regular pointer to a function pointer. -Russell
int (*cmd_funcs[NCMDS+1]) (char *cmd, char *args, char **qmsg, fd_set *master, int *fdmax, int flag);
char *commands[NCMDS+1];


int init_cmds()
{

	cmd_funcs[NCMDS - 1] = NULL;
	commands[NCMDS - 1] = NULL;

	START_ADDING_CMDS(); //initializes the command counter

	ADD_CMD("quit", CMD_FNAME(quit));
	ADD_CMD("exit", CMD_FNAME(quit));

	ADD_CMD("close", CMD_FNAME(close));

	ADD_CMD("log", CMD_FNAME(log));

	ADD_CMD("set", CMD_FNAME(set));

	ADD_CMD("server", CMD_FNAME(server));
	ADD_CMD("connect", CMD_FNAME(server));

	ADD_CMD("reconnect", CMD_FNAME(reconnect));
	
	ADD_CMD("disconnect", CMD_FNAME(disconnect));

	ADD_CMD("realsname", CMD_FNAME(realsname));
	
	ADD_CMD("join", CMD_FNAME(join));

	ADD_CMD("rejoin", CMD_FNAME(rejoin));
	
	ADD_CMD("part", CMD_FNAME(part));
	ADD_CMD("leave", CMD_FNAME(part));
	
	ADD_CMD("unaway", CMD_FNAME(unaway));
	
	ADD_CMD("away", CMD_FNAME(away));
	
	ADD_CMD("afk", CMD_FNAME(afk));
	
	ADD_CMD("nick", CMD_FNAME(nick));
	
	ADD_CMD("topic", CMD_FNAME(topic));
	
	ADD_CMD("msg", CMD_FNAME(msg));
	
	ADD_CMD("ping", CMD_FNAME(ping));
	
	ADD_CMD("amsg", CMD_FNAME(amsg));
	
	ADD_CMD("me", CMD_FNAME(me));
	
	ADD_CMD("tab", CMD_FNAME(tab));
	
	ADD_CMD("sort", CMD_FNAME(sort));
	
	ADD_CMD("left", CMD_FNAME(left));
	
	ADD_CMD("right", CMD_FNAME(right));
	
	ADD_CMD("ignore", CMD_FNAME(ignore));
	
	ADD_CMD("mode", CMD_FNAME(mode));
	
	ADD_CMD("cmd", CMD_FNAME(cmd));
	ADD_CMD("quote", CMD_FNAME(cmd));

	//Verify that the number of commands added
	//is the number expected.
	assert(__i==NCMDS);

	return(0);

}

int call_cmd(char *cmd, char *args, char **qmsg, fd_set *master, int *fdmax)
{	int i = 0;
	char *ccmd = commands[0];
	
	while(ccmd){
		if(strcmp(ccmd,cmd)==0){
			return cmd_funcs[i](cmd,args,qmsg,master,fdmax,0);
		}
		i++;
		ccmd=commands[i];
	}
	
	if(!cmd) cmd="";
	char dstr[8+strlen(cmd)];
	sprintf(dstr, "/%s: ", cmd);
	add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Unrecognised command!", dstr);
	return 0;
}

//commands may not have args or use the original cmd 
//so we have to turn of warnings for them so that
//they don't get caught in -Werror -Russell
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

CMD_FUN(quit){
		if(args) {free(*qmsg); *qmsg=strdup(args);}
		add_to_buffer(cbuf, STA, NORMAL, 0, false, "Exited quirc", "/quit: ");
		return(-1);
}

CMD_FUN(close){
		switch(bufs[cbuf].type)
		{
			case STATUS:
				cmd="quit";
			break;
			case SERVER:
				if(bufs[cbuf].live)
				{
					cmd="disconnect";
				}
				else
				{
					free_buffer(cbuf);
					return(0);
				}
			break;
			case CHANNEL:
				if(bufs[cbuf].live)
				{
					cmd="part";
				}
				else
				{
					free_buffer(cbuf);
					return(0);
				}
			break;
			default:
				bufs[cbuf].live=false;
				free_buffer(cbuf);
				return(0);
			break;
		}
		return -1;
}

CMD_FUN(log){
		if(!args)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a log type or /log -", "/log: ");
			return(0);
		}
		if(bufs[cbuf].logf)
		{
			fclose(bufs[cbuf].logf);
			bufs[cbuf].logf=NULL;
		}
		if(strcmp(args, "-")==0)
		{
			add_to_buffer(cbuf, STA, QUIET, 0, false, "Disabled logging of this buffer", "/log: ");
			return(0);
		}
		else
		{
			char *type=strtok(args, " ");
			if(type)
			{
				char *fn=strtok(NULL, "");
				if(fn)
				{
					logtype logt;
					if(strcasecmp(type, "plain")==0)
						logt=LOGT_PLAIN;
					else if(strcasecmp(type, "symbolic")==0)
						logt=LOGT_SYMBOLIC;
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Unrecognised log type (valid types are: plain, symbolic)", "/log: ");
						return(0);
					}
					FILE *fp=fopen(fn, "a");
					if(!fp)
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Failed to open log file for append", "/log: ");
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, strerror(errno), "fopen: ");
						return(0);
					}
					log_init(fp, logt);
					bufs[cbuf].logf=fp;
					bufs[cbuf].logt=logt;
					add_to_buffer(cbuf, STA, QUIET, 0, false, "Enabled logging of this buffer", "/log: ");
					return(0);
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a log file", "/log: ");
					return(0);
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a log type or /log -", "/log: ");
				return(0);
			}
		}
}
CMD_FUN(set){
		bool osp=show_prefix, odbg=debug, oind=indent;
		unsigned int omln=maxnlen;
		if(args)
		{
			char *opt=strtok(args, " ");
			if(opt)
			{
				char *val=strtok(NULL, "");
#include "config_set.c"
				else if(strcmp(opt, "conf")==0)
				{
					if(bufs[cbuf].type==CHANNEL)
					{
						if(val)
						{
							if(isdigit(*val))
							{
								unsigned int value;
								sscanf(val, "%u", &value);
								bufs[cbuf].conf=value;
							}
							else if(strcmp(val, "+")==0)
							{
								bufs[cbuf].conf=true;
							}
							else if(strcmp(val, "-")==0)
							{
								bufs[cbuf].conf=false;
							}
							else
							{
								add_to_buffer(cbuf, ERR, NORMAL, 0, false, "option 'conf' is boolean, use only 0/1 or -/+ to set", "/set: ");
							}
						}
						else
							bufs[cbuf].conf=true;
						if(bufs[cbuf].conf)
							add_to_buffer(cbuf, STA, QUIET, 0, false, "conference mode enabled for this channel", "/set: ");
						else
							add_to_buffer(cbuf, STA, QUIET, 0, false, "conference mode disabled for this channel", "/set: ");
						mark_buffer_dirty(cbuf);
						redraw_buffer();
					}
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Not a channel!", "/set conf: ");
					}
				}
				else if(strcmp(opt, "uname")==0)
				{
					if(val)
					{
						free(username);
						username=strdup(val);
						add_to_buffer(cbuf, STA, QUIET, 0, false, username, "/set uname ");
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Non-null value required for uname", "/set uname: ");
				}
				else if(strcmp(opt, "fname")==0)
				{
					if(val)
					{
						free(fname);
						fname=strdup(val);
						add_to_buffer(cbuf, STA, QUIET, 0, false, fname, "/set fname ");
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Non-null value required for fname", "/set fname: ");
				}
				else if(strcmp(opt, "pass")==0)
				{
					if(val)
					{
						free(pass);
						pass=strdup(val);
						char *p=val;
						while(*p) *p++='*';
						add_to_buffer(cbuf, STA, QUIET, 0, false, val, "/set pass ");
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Non-null value required for pass", "/set pass: ");
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "No such option!", "/set: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "But what do you want to set?", "/set: ");
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "But what do you want to set?", "/set: ");
		}
		if((show_prefix!=osp)||(maxnlen!=omln)||(indent!=oind))
		{
			for(int b=0;b<nbufs;b++)
				mark_buffer_dirty(b);
		}
		if(debug&&!odbg)
		{
			push_ring(&d_buf, DEBUG);
		}
		else if(odbg&&!debug)
		{
			init_ring(&d_buf);
			d_buf.loop=true;
		}
		return(0);
	}
CMD_FUN(server){
		if(args)
		{
			char *server=args;
			char *newport=strchr(server, ':');
			if(newport)
			{
				*newport=0;
				newport++;
			}
			else
			{
				newport=portno;
			}
			int b;
			for(b=1;b<nbufs;b++)
			{
				if((bufs[b].type==SERVER) && (irc_strcasecmp(server, bufs[b].bname, bufs[b].casemapping)==0))
				{
					if(bufs[b].live)
					{
						cbuf=b;
						redraw_buffer();
						return(0);
					}
					else
					{
						cbuf=b;
						cmd="reconnect";
						redraw_buffer();
						break;
					}
				}
			}
			if(b>=nbufs)
			{
				char dstr[30+strlen(server)+strlen(newport)];
				sprintf(dstr, "Connecting to %s on port %s...", server, newport);
				#if ASYNCH_NL
				__attribute__((unused)) int *p= fdmax;
				nl_list *nl=irc_connect(server, newport);
				if(nl)
				{
					nl->reconn_b=0;
					add_to_buffer(0, STA, QUIET, 0, false, dstr, "/server: ");
					redraw_buffer();
				}
				#else
				int serverhandle=irc_connect(server, newport, master, fdmax);
				if(serverhandle)
				{
					bufs=(buffer *)realloc(bufs, ++nbufs*sizeof(buffer));
					init_buffer(nbufs-1, SERVER, server, buflines);
					cbuf=nbufs-1;
					bufs[cbuf].handle=serverhandle;
					bufs[cbuf].nick=bufs[0].nick?strdup(bufs[0].nick):NULL;
					bufs[cbuf].server=cbuf;
					bufs[cbuf].conninpr=true;
					add_to_buffer(cbuf, STA, QUIET, 0, false, dstr, "/server: ");
					redraw_buffer();
				}
				#endif
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a server!", "/server: ");
		}
		return(0);
	}
CMD_FUN(reconnect){
		if(bufs[cbuf].server)
		{
			if(!SERVER(cbuf).live)
			{
				char *newport;
				if(args)
				{
					newport=args;
				}
				else if(SERVER(cbuf).autoent && SERVER(cbuf).autoent->portno)
				{
					newport=SERVER(cbuf).autoent->portno;
				}
				else
				{
					newport=portno;
				}
				char dstr[30+strlen(SERVER(cbuf).serverloc)+strlen(newport)];
				sprintf(dstr, "Connecting to %s on port %s...", SERVER(cbuf).serverloc, newport);
				#if ASYNCH_NL
				nl_list *nl=irc_connect(SERVER(cbuf).serverloc, newport);
				if(nl)
				{
					nl->reconn_b=bufs[cbuf].server;
					add_to_buffer(bufs[cbuf].server, STA, QUIET, 0, false, dstr, "/server: ");
					redraw_buffer();
				}
				else
				{
					add_to_buffer(bufs[cbuf].server, ERR, NORMAL, 0, false, "malloc failure (see status)", "/server: ");
					redraw_buffer();
				}
				#else /* ASYNCH_NL */
				int serverhandle=irc_connect(SERVER(cbuf).serverloc, newport, master, fdmax);
				if(serverhandle)
				{
					int b=bufs[cbuf].server;
					bufs[b].handle=serverhandle;
					int b2;
					for(b2=1;b2<nbufs;b2++)
					{
						if(bufs[b2].server==b)
							bufs[b2].handle=serverhandle;
					}
					bufs[cbuf].conninpr=true;
					free(bufs[cbuf].realsname);
					bufs[cbuf].realsname=NULL;
					add_to_buffer(cbuf, STA, QUIET, 0, false, dstr, "/server: ");
				}
				#endif /* ASYNCH_NL */
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Already connected to server", "/reconnect: ");
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/reconnect: ");
		}
		return(0);
	}
CMD_FUN(disconnect){
		int b=bufs[cbuf].server;
		if(b>0)
		{
			if(bufs[b].handle)
			{
				if(bufs[b].live)
				{
					char quit[7+strlen(args?args:(qmsg&&*qmsg)?*qmsg:"quIRC quit")];
					sprintf(quit, "QUIT %s", args?args:(qmsg&&*qmsg)?*qmsg:"quIRC quit");
					irc_tx(bufs[b].handle, quit);
				}
				close(bufs[b].handle);
				FD_CLR(bufs[b].handle, master);
			}
			int b2;
			for(b2=1;b2<nbufs;b2++)
			{
				while((b2<nbufs) && (bufs[b2].type!=SERVER) && ((bufs[b2].server==b) || (bufs[b2].server==0)))
				{
					bufs[b2].live=false;
					free_buffer(b2);
				}
			}
			bufs[b].live=false;
			free_buffer(b);
			cbuf=0;
			redraw_buffer();
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't disconnect (status)!", "/disconnect: ");
		}
		return(0);
	}
CMD_FUN(realsname){
		int b=bufs[cbuf].server;
		if(b>0)
		{
			if(bufs[b].realsname)
				add_to_buffer(cbuf, STA, NORMAL, 0, false, bufs[b].realsname, "/realsname: ");
			else
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "unknown", "/realsname ");
		}
		else
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "(status) is not a server", "/realsname: ");
		return(0);
	}
CMD_FUN(join){
		if(!SERVER(cbuf).handle)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/join: ");
		}
		else if(!SERVER(cbuf).live)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Disconnected, can't send", "/join: ");
		}
		else if(args)
		{
			char *chan=strtok(args, " ");
			if(!chan)
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a channel!", "/join: ");
			}
			else
			{
				char *pass=strtok(NULL, ", ");
				servlist *serv=SERVER(cbuf).autoent;
				if(!serv)
				{
					serv=SERVER(cbuf).autoent=malloc(sizeof(servlist));
					serv->name=NULL;
					serv->portno=NULL;
					serv->nick=NULL;
					serv->pass=NULL;
					serv->chans=NULL;
					serv->next=NULL;
					serv->igns=NULL;
				}
				if(pass)
				{
					chanlist *curr=malloc(sizeof(chanlist));
					if(curr)
					{
						curr->name=strdup(chan);
						curr->key=strdup(pass);
						curr->next=serv->chans;
						serv->chans=curr;
					}
				}
				else
				{
					chanlist *curr=serv->chans;
					while(curr)
					{
						if(irc_strcasecmp(curr->name, chan, SERVER(cbuf).casemapping)==0)
						{
							pass=curr->key;
							break;
						}
						curr=curr->next;
					}
				}
				if(!pass) pass="";
				char joinmsg[8+strlen(chan)+strlen(pass)];
				sprintf(joinmsg, "JOIN %s %s", chan, pass);
				irc_tx(SERVER(cbuf).handle, joinmsg);
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a channel!", "/join: ");
		}
		return(0);
	}
CMD_FUN(rejoin){
		if(bufs[cbuf].type==PRIVATE)
		{
			if(!(SERVER(cbuf).handle && SERVER(cbuf).live))
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Disconnected, can't send", "/rejoin: ");
			}
			else if(bufs[cbuf].live)
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Already in this channel", "/rejoin: ");
			}
			else
			{
				bufs[cbuf].live=true;
			}
		}
		else if(bufs[cbuf].type!=CHANNEL)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "View is not a channel!", "/rejoin: ");
		}
		else if(!(SERVER(cbuf).handle && SERVER(cbuf).live))
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Disconnected, can't send", "/rejoin: ");
		}
		else if(bufs[cbuf].live)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Already in this channel", "/rejoin: ");
		}
		else
		{
			char *chan=bufs[cbuf].bname;
			char *pass=args;
			if(pass)
				bufs[cbuf].lastkey=strdup(pass);
			else
				pass=bufs[cbuf].key;
			if(!pass) pass="";
			char joinmsg[8+strlen(chan)+strlen(pass)];
			sprintf(joinmsg, "JOIN %s %s", chan, pass);
			irc_tx(SERVER(cbuf).handle, joinmsg);
			redraw_buffer();
		}
		return(0);
	}
CMD_FUN(part){
		if(bufs[cbuf].type!=CHANNEL)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "This view is not a channel!", "/part: ");
		}
		else
		{
			if(LIVE(cbuf) && SERVER(cbuf).handle)
			{
				char partmsg[8+strlen(bufs[cbuf].bname)];
				sprintf(partmsg, "PART %s", bufs[cbuf].bname);
				irc_tx(SERVER(cbuf).handle, partmsg);
				add_to_buffer(cbuf, PART, NORMAL, 0, true, "Leaving", "/part: ");
			}
			// when you try to /part a dead tab, interpret it as a /close
			int parent=bufs[cbuf].server;
			bufs[cbuf].live=false;
			free_buffer(cbuf);
			cbuf=parent;
			redraw_buffer();
		}
		return(0);
	}
CMD_FUN(unaway){
		return CMD_FNAME(away)("away","-",qmsg,master,fdmax,0);
	}
CMD_FUN(away){
		const char *am="Gone away, gone away, was it one of you took it away?";
		if(args)
			am=args;
		if(SERVER(cbuf).handle)
		{
			if(LIVE(cbuf))
			{
				if(strcmp(am, "-"))
				{
					char nmsg[8+strlen(am)];
					sprintf(nmsg, "AWAY :%s", am);
					irc_tx(SERVER(cbuf).handle, nmsg);
				}
				else
				{
					irc_tx(SERVER(cbuf).handle, "AWAY"); // unmark away
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/away: ");
			}
		}
		else
		{
			if(cbuf)
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/away: ");
			else
			{
				int b;
				for(b=0;b<nbufs;b++)
				{
					if(bufs[b].type==SERVER)
					{
						if(bufs[b].handle)
						{
							if(LIVE(b))
							{
								if(strcmp(am, "-"))
								{
									char nmsg[8+strlen(am)];
									sprintf(nmsg, "AWAY :%s", am);
									irc_tx(bufs[b].handle, nmsg);
								}
								else
								{
									irc_tx(bufs[b].handle, "AWAY"); // unmark away
								}
							}
							else
								add_to_buffer(b, ERR, NORMAL, 0, false, "Tab not live, can't send", "/away: ");
						}
						else
							add_to_buffer(b, ERR, NORMAL, 0, false, "Tab not live, can't send", "/away: ");
					}
				}
			}
		}
		return(0);
	}
CMD_FUN(afk){
		int aalloc = false;
		const char *afm="afk";
		if(args)
			afm=strtok(args, " ");
		const char *p=SERVER(cbuf).nick;
		int n=strcspn(p, "|");
		char *nargs=malloc(n+strlen(afm)+2);
		if(nargs)
		{
			cmd="nick";
			strncpy(nargs, p, n);
			nargs[n]=0;
			if(strcmp(afm, "-"))
			{
				strcat(nargs, "|");
				strcat(nargs, afm);
			}
			args=nargs;
			aalloc=true;
		}
		return CMD_FNAME(nick)(cmd,args,qmsg,master,fdmax,aalloc);

	}
CMD_FUN(nick){
		if(args)
		{
			char *nn=strtok(args, " ");
			if(SERVER(cbuf).handle)
			{
				if(LIVE(cbuf))
				{
					free(SERVER(cbuf).nick);
					SERVER(cbuf).nick=strdup(nn);
					char nmsg[8+strlen(SERVER(cbuf).nick)];
					sprintf(nmsg, "NICK %s", SERVER(cbuf).nick);
					irc_tx(SERVER(cbuf).handle, nmsg);
					add_to_buffer(cbuf, STA, QUIET, 0, false, "Changing nick", "/nick: ");
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/nick: ");
				}
			}
			else
			{
				if(cbuf)
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/nick: ");
				else
				{
					free(bufs[0].nick);
					bufs[0].nick=strdup(nn);
					defnick=false;
					add_to_buffer(cbuf, STA, QUIET, 0, false, "Default nick changed", "/nick: ");
				}
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a nickname!", "/nick: ");
		}
		if(flag) free(args);
		return(0);
	}
CMD_FUN(topic){
		if(args)
		{
			if(bufs[cbuf].type==CHANNEL)
			{
				if(SERVER(cbuf).handle)
				{
					if(LIVE(cbuf))
					{
						char tmsg[10+strlen(bufs[cbuf].bname)+strlen(args)];
						sprintf(tmsg, "TOPIC %s :%s", bufs[cbuf].bname, args);
						irc_tx(SERVER(cbuf).handle, tmsg);
					}
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/topic: ");
					}
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't send to channel - not connected!", "/topic: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't set topic - view is not a channel!", "/topic: ");
			}
		}
		else
		{
			if(bufs[cbuf].type==CHANNEL)
			{
				if(SERVER(cbuf).handle)
				{
					if(LIVE(cbuf))
					{
						char tmsg[8+strlen(bufs[cbuf].bname)];
						sprintf(tmsg, "TOPIC %s", bufs[cbuf].bname);
						irc_tx(SERVER(cbuf).handle, tmsg);
					}
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/topic: ");
					}
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't send to channel - not connected!", "/topic: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't get topic - view is not a channel!", "/topic: ");
			}
		}
		return(0);
	}
CMD_FUN(msg){
		if(!SERVER(cbuf).handle)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/msg: ");
		}
		else if(args)
		{
			bool no_tab=false;
			char *dest=strtok(args, " ");
			if(strcmp(dest, "-n")==0)
			{
				no_tab=true;
				dest=strtok(NULL, " ");
			}
			if(!dest)
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a recipient!", "/msg: ");
				return(0);
			}
			char *text=strtok(NULL, "");
			if(!no_tab)
			{
				int b2=makeptab(bufs[cbuf].server, dest);
				cbuf=b2;
				redraw_buffer();
			}
			if(text)
			{
				if(SERVER(cbuf).handle)
				{
					if(LIVE(cbuf))
					{
						char privmsg[12+strlen(dest)+strlen(text)];
						sprintf(privmsg, "PRIVMSG %s :%s", dest, text);
						irc_tx(SERVER(cbuf).handle, privmsg);
						ctcp_strip(text, SERVER(cbuf).nick, cbuf, false, false, true, true);
						if(no_tab)
						{
							add_to_buffer(cbuf, STA, QUIET, 0, false, "sent", "/msg -n: ");
						}
						else
						{
							while(text[strlen(text)-1]=='\n')
								text[strlen(text)-1]=0; // stomp out trailing newlines, they break things
							add_to_buffer(cbuf, MSG, NORMAL, 0, true, text, SERVER(cbuf).nick);
						}
					}
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/msg: ");
					}
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't send to channel - not connected!", "/msg: ");
				}
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a recipient!", "/msg: ");
		}
		return(0);
	}
CMD_FUN(ping){
		if(!SERVER(cbuf).handle)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/ping: ");
		}
		else
		{
			const char *dest=NULL;
			if(args)
				dest=strtok(args, " ");
			if(!dest&&bufs[cbuf].type==PRIVATE)
				dest=bufs[cbuf].bname;
			if(dest)
			{
				if(SERVER(cbuf).handle)
				{
					if(LIVE(cbuf))
					{
						struct timeval tv;
						gettimeofday(&tv, NULL);
						char privmsg[64+strlen(dest)];
						snprintf(privmsg, 64+strlen(dest), "PRIVMSG %s :\001PING %u %u\001", dest, (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);
						irc_tx(SERVER(cbuf).handle, privmsg);
					}
					else
					{
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/ping: ");
					}
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't send - not connected!", "/ping: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a recipient!", "/ping: ");
			}
		}
		return(0);
	}
CMD_FUN(amsg){
		if(!bufs[cbuf].server)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/amsg: ");
		}
		else if(args)
		{
			int b2;
			for(b2=1;b2<nbufs;b2++)
			{
				if((bufs[b2].server==bufs[cbuf].server) && (bufs[b2].type==CHANNEL))
				{
					if(LIVE(b2))
					{
						char privmsg[12+strlen(bufs[b2].bname)+strlen(args)];
						sprintf(privmsg, "PRIVMSG %s :%s", bufs[b2].bname, args);
						irc_tx(bufs[b2].handle, privmsg);
						while(args[strlen(args)-1]=='\n')
							args[strlen(args)-1]=0; // stomp out trailing newlines, they break things
						bool al=bufs[b2].alert; // save alert status...
						int hi=bufs[b2].hi_alert;
						add_to_buffer(b2, MSG, NORMAL, 0, true, args, SERVER(b2).nick);
						bufs[b2].alert=al; // and restore it
						bufs[b2].hi_alert=hi;
					}
					else
					{
						add_to_buffer(b2, ERR, NORMAL, 0, false, "Tab not live, can't send", "/amsg: ");
					}
				}
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a message!", "/amsg: ");
		}
		return(0);
	}
CMD_FUN(me){
		if(!((bufs[cbuf].type==CHANNEL)||(bufs[cbuf].type==PRIVATE)))
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't talk here, not a channel/private chat", "/me: ");
		}
		else if(args)
		{
			if(SERVER(cbuf).handle)
			{
				if(LIVE(cbuf))
				{
					char privmsg[32+strlen(bufs[cbuf].bname)+strlen(args)];
					sprintf(privmsg, "PRIVMSG %s :\001ACTION %s\001", bufs[cbuf].bname, args);
					irc_tx(SERVER(cbuf).handle, privmsg);
					while(args[strlen(args)-1]=='\n')
						args[strlen(args)-1]=0; // stomp out trailing newlines, they break things
					add_to_buffer(cbuf, ACT, NORMAL, 0, true, args, SERVER(cbuf).nick);
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/me: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't send to channel - not connected!", "/msg: ");
			}
		}
		else
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify an action!", "/me: ");
		}
		return(0);
	}
CMD_FUN(tab){
		if(!args)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a tab!", "/tab: ");
		}
		else
		{
			int bufn;
			if(sscanf(args, "%d", &bufn)==1)
			{
				if((bufn>=0) && (bufn<nbufs))
				{
					cbuf=bufn;
					redraw_buffer();
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "No such tab!", "/tab: ");
				}
			}
			else
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must specify a tab!", "/tab: ");
			}
		}
		return(0);
	}
CMD_FUN(sort){
		int newbufs[nbufs];
		buffer bi[nbufs];
		newbufs[0]=0;
		int b,buf=1;
		for(b=0;b<nbufs;b++)
		{
			bi[b]=bufs[b];
			if(bufs[b].type==SERVER)
			{
				newbufs[buf++]=b;
				int b2;
				for(b2=1;b2<nbufs;b2++)
				{
					if((bufs[b2].server==b)&&(b2!=b))
					{
						newbufs[buf++]=b2;
					}
				}
			}
		}
		if(buf!=nbufs)
		{
			add_to_buffer(cbuf, ERR, QUIET, 0, false, "Internal error (bad count)", "/sort: ");
			return(0);
		}
		int serv=0, cb=0;
		for(b=0;b<nbufs;b++)
		{
			bufs[b]=bi[newbufs[b]];
			if(bufs[b].type==SERVER)
				serv=b;
			bufs[b].server=serv;
			if(newbufs[b]==cbuf)
				cb=b;
		}
		cbuf=cb;
		redraw_buffer();
		return(0);
	}
CMD_FUN(left){
		if(cbuf<2)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't move (status) tab!", "/left: ");
		}
		else
		{
			buffer tmp=bufs[cbuf];
			bufs[cbuf]=bufs[cbuf-1];
			bufs[cbuf-1]=tmp;
			cbuf--;
			int i;
			for(i=0;i<nbufs;i++)
			{
				if(bufs[i].server==cbuf)
				{
					bufs[i].server++;
				}
				else if(bufs[i].server==cbuf+1)
				{
					bufs[i].server--;
				}
			}
			redraw_buffer();
		}
		return(0);
	}
CMD_FUN(right){
		if(!cbuf)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Can't move (status) tab!", "/right: ");
		}
		else if(cbuf==nbufs-1)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Nowhere to move to!", "/right: ");
		}
		else
		{
			buffer tmp=bufs[cbuf];
			bufs[cbuf]=bufs[cbuf+1];
			bufs[cbuf+1]=tmp;
			cbuf++;
			int i;
			for(i=0;i<nbufs;i++)
			{
				if(bufs[i].server==cbuf-1)
				{
					bufs[i].server++;
				}
				else if(bufs[i].server==cbuf)
				{
					bufs[i].server--;
				}
			}
			redraw_buffer();
		}
		return(0);
	}
CMD_FUN(ignore){
		if(!args)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Missing arguments!", "/ignore: ");
		}
		else
		{
			char *arg=strtok(args, " ");
			bool regex=false;
			bool icase=false;
			bool pms=false;
			bool del=false;
			while(arg)
			{
				if(*arg=='-')
				{
					if(strcmp(arg, "-i")==0)
					{
						icase=true;
					}
					else if(strcmp(arg, "-r")==0)
					{
						regex=true;
					}
					else if(strcmp(arg, "-d")==0)
					{
						del=true;
					}
					else if(strcmp(arg, "-p")==0)
					{
						pms=true;
					}
					else if(strcmp(arg, "-l")==0)
					{
						i_list();
						break;
					}
					else if(strcmp(arg, "--")==0)
					{
						arg=strtok(NULL, "");
						continue;
					}
				}
				else
				{
					if(del)
					{
						if(i_cull(&bufs[cbuf].ilist, arg))
						{
							add_to_buffer(cbuf, STA, QUIET, 0, false, "Entries deleted", "/ignore -d: ");
						}
						else
						{
							add_to_buffer(cbuf, ERR, NORMAL, 0, false, "No entries deleted", "/ignore -d: ");
						}
					}
					else if(regex)
					{
						name *new=n_add(&bufs[cbuf].ilist, arg, bufs[cbuf].casemapping);
						if(new)
						{
							add_to_buffer(cbuf, STA, QUIET, 0, false, "Entry added", "/ignore: ");
							new->icase=icase;
							new->pms=pms;
						}
					}
					else
					{
						char *isrc,*iusr,*ihst;
						prefix_split(arg, &isrc, &iusr, &ihst);
						if((!isrc) || (*isrc==0) || (*isrc=='*'))
							isrc="[^!@]*";
						if((!iusr) || (*iusr==0) || (*iusr=='*'))
							iusr="[^!@]*";
						if((!ihst) || (*ihst==0) || (*ihst=='*'))
							ihst="[^@]*";
						char expr[16+strlen(isrc)+strlen(iusr)+strlen(ihst)];
						sprintf(expr, "^%s[_~]*!%s@%s$", isrc, iusr, ihst);
						name *new=n_add(&bufs[cbuf].ilist, expr, bufs[cbuf].casemapping);
						if(new)
						{
							add_to_buffer(cbuf, STA, QUIET, 0, false, "Entry added", "/ignore: ");
							new->icase=icase;
							new->pms=pms;
						}
					}
					break;
				}
				arg=strtok(NULL, " ");
			}
		}
		return(0);
	}
CMD_FUN(mode){
		if(!SERVER(cbuf).handle)
		{
			add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/mode: ");
		}
		else
		{
			// /mode [{<user>|<banmask>|<limit>} [{\+|-}[[:alpha:]]+]]
			if(args)
			{
				char *user=strtok(args, " ");
				char *modes=strtok(NULL, "");
				if(modes)
				{
					if(LIVE(cbuf))
					{
						if(bufs[cbuf].type==CHANNEL)
						{
							char mmsg[8+strlen(bufs[cbuf].bname)+strlen(modes)+strlen(user)];
							sprintf(mmsg, "MODE %s %s %s", bufs[cbuf].bname, modes, user);
							irc_tx(SERVER(cbuf).handle, mmsg);
						}
						else
							add_to_buffer(cbuf, ERR, NORMAL, 0, false, "This is not a channel", "/mode: ");
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/mode: ");
				}
				else
				{
					if(bufs[cbuf].type==CHANNEL)
					{
						name *curr=bufs[cbuf].nlist;
						while(curr)
						{
							if(irc_strcasecmp(curr->data, user, SERVER(cbuf).casemapping)==0)
							{
								if(curr==bufs[cbuf].us) goto youmode;
								char mm[12+strlen(curr->data)+curr->npfx];
								int mpos=0;
								sprintf(mm, "%s has mode %n-", curr->data, &mpos);
								if(mpos)
								{
									for(unsigned int i=0;i<curr->npfx;i++)
										mm[mpos++]=curr->prefixes[i].letter;
									if(curr->npfx) mm[mpos]=0;
									add_to_buffer(cbuf, MODE, NORMAL, 0, false, mm, "/mode: ");
								}
								else
									add_to_buffer(cbuf, ERR, NORMAL, 0, false, "\"Impossible\" error (mpos==0)", "/mode: ");
								break;
							}
							curr=curr->next;
						}
						if(!curr)
						{
							char mm[16+strlen(user)];
							sprintf(mm, "No such nick: %s", user);
							add_to_buffer(cbuf, ERR, NORMAL, 0, false, mm, "/mode: ");
						}
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "This is not a channel", "/mode: ");
				}
			}
			else
			{
				youmode:
				if(bufs[cbuf].type==CHANNEL)
				{
					if(bufs[cbuf].us)
					{
						char mm[20+strlen(SERVER(cbuf).nick)+bufs[cbuf].us->npfx];
						int mpos=0;
						sprintf(mm, "You (%s) have mode %n-", SERVER(cbuf).nick, &mpos);
						if(mpos)
						{
							for(unsigned int i=0;i<bufs[cbuf].us->npfx;i++)
								mm[mpos++]=bufs[cbuf].us->prefixes[i].letter;
							if(bufs[cbuf].us->npfx) mm[mpos]=0;
							add_to_buffer(cbuf, MODE, NORMAL, 0, true, mm, "/mode: ");
						}
						else
							add_to_buffer(cbuf, ERR, NORMAL, 0, false, "\"Impossible\" error (mpos==0)", "/mode: ");
					}
					else
						add_to_buffer(cbuf, ERR, NORMAL, 0, false, "\"Impossible\" error (us==NULL)", "/mode: ");
				}
				else
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "This is not a channel", "/mode: ");
			}
		}
		return(0);
	}
CMD_FUN(cmd){
		if(args)
		{
			if(!SERVER(cbuf).handle)
			{
				add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Must be run in the context of a server!", "/cmd: ");
			}
			else	 
			{
				bool force=false;
				if(strncmp(args, "-f", 2)==0)
				{
					force=true;
					args++;
					while (*++args==' ');
				}
				if(force||LIVE(cbuf))
				{
					irc_tx(SERVER(cbuf).handle, args);
					add_to_buffer(cbuf, STA, NORMAL, 0, false, args, "/cmd: ");
				}
				else
				{
					add_to_buffer(cbuf, ERR, NORMAL, 0, false, "Tab not live, can't send", "/cmd: ");
				}
			}
		}
		return(0);
	}
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

