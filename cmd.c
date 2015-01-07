
#include "buffer.h"
#include "cmd.h"
#include "logging.h"

CMD_FUN(quit);
CMD_FUN(close);
CMD_FUN(log);
CMD_FUN(set);
CMD_FUN(server);

//Commands
//Remember to increment NCMDS when adding commands. -Russell
#define NCMDS 7

//cmd_funcs can't be malloc'd the regular way because of -Werror
//malloc returns a void pointer and with -Werror you can't
//cast from a regular pointer to a function pointer. -Russell
int (*cmd_funcs[NCMDS+1]) (char *cmd, char *args, char **qmsg, int *fdmax);
char *commands[NCMDS+1];


void init_cmds()
{

	cmd_funcs[NCMDS - 1] = NULL;
	commands[NCMDS - 1] = NULL;

	START_ADDING_CMDS();
	ADD_CMD("quit", CMD_FNAME(quit));
	ADD_CMD("exit", CMD_FNAME(quit));
	ADD_CMD("close", CMD_FNAME(close));
	ADD_CMD("log", CMD_FNAME(log));
	ADD_CMD("set", CMD_FNAME(set));
	ADD_CMD("server", CMD_FNAME(server));
	ADD_CMD("connect", CMD_FNAME(server));


}

int call_cmd(char *cmd, char *args, char **qmsg, int *fdmax)
{	int i = 0;
	char *ccmd = commands[0];
	
	while(ccmd){
		if(!strcmp(ccmd,cmd)){
			return cmd_funcs[i](cmd,args,qmsg,fdmax);
		}
		i++;
		ccmd++;
	}	
	return -1;
}

//commands may not have args or use the original command 
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
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

