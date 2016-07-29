#include "cmd.h"
#include <sys/types.h>
#include <regex.h>
#include <assert.h>
#include "logging.h"
#include "strbuf.h"
#include "types.h"

//Function declarations
CMD_FUN (ambig); // ambiguous command handler
CMD_FUN (quit);
CMD_FUN (close);
CMD_FUN (log);
CMD_FUN (set);
CMD_FUN (server);
CMD_FUN (reconnect);
CMD_FUN (disconnect);
CMD_FUN (realsname);
CMD_FUN (join);
CMD_FUN (rejoin);
CMD_FUN (part);
CMD_FUN (unaway);
CMD_FUN (away);
CMD_FUN (afk);
CMD_FUN (nick);
CMD_FUN (topic);
CMD_FUN (msg);
CMD_FUN (ping);
CMD_FUN (amsg);
CMD_FUN (me);
CMD_FUN (tab);
CMD_FUN (sort);
CMD_FUN (left);
CMD_FUN (right);
CMD_FUN (ignore);
CMD_FUN (mode);
CMD_FUN (grep);
CMD_FUN (cmd);
CMD_FUN (help);

unsigned int ncmds;
struct cmd_t *commands;

name *cmds_as_nlist = NULL;

int add_cmd(char *cmd_name, cmd_func func, char *help)
{
	unsigned int n = ncmds++;
	struct cmd_t *nc = realloc(commands, ncmds * sizeof(struct cmd_t));
	if (nc == NULL) {
		fprintf(stderr, "Failed to initialise commands: %s\n", strerror(errno));
		return -1;
	}
	(commands = nc)[n] = (struct cmd_t){.name=cmd_name, .func=func, .help=help};
	name *as_name = malloc(sizeof(name));
	if (as_name == NULL) {
		perror("malloc");
		return -1;
	}
	as_name->next = cmds_as_nlist;
	as_name->prev = NULL;
	as_name->data = strdup(cmd_name);
	as_name->npfx = 0;
	as_name->prefixes = NULL;
	cmds_as_nlist = as_name;
	return 0;
}

#define ADD_CMD(_name, _fname, _help) do {if (add_cmd(_name, CMD_FNAME(_fname), _help)) return -1;} while(0);

int init_cmds()
{
	ncmds = 0;
	commands = NULL;

	ADD_CMD ("quit", quit, "/quit\nQuit quIRC");
	ADD_CMD ("exit", quit, "/exit\nQuit quIRC");

	ADD_CMD ("close", close, "/close\nClose the current tab.");

	ADD_CMD ("log", log, "/log <logtype> <file>\tStart logging the current buffer.\n/log - \tDisable logging for the current buffer.");

	ADD_CMD ("set", set, "/set <option> [value]\nSet configuration values.");

	ADD_CMD ("server", server, "/server <url> [<pass>]\nConnect to the given server.");
	ADD_CMD ("connect", server, "/connect <url> [<pass>]\nConnect to the given server.");

	ADD_CMD ("reconnect", reconnect, "/reconnect\nReconnects to a server which has become disconnected.");

	ADD_CMD ("disconnect", disconnect, "/disconnect [msg]\nDisconnect from the current server and leave a message.");

	ADD_CMD ("realsname", realsname, "/realsname\nDisplays hostname of the current server.");

	ADD_CMD ("join", join, "/join <channel> [key]\nJoin a channel.");

	ADD_CMD ("rejoin", rejoin, "/rejoin [key]\nRejoins a dead channel tab.");

	ADD_CMD ("part", part, "/part <channel>\nLeave a channel.");
	ADD_CMD ("leave", part, "/leave <channel>\nLeave a channel.");

	ADD_CMD ("unaway", unaway, "/unaway\nUnset away message."); ADD_CMD ("back", unaway, "/back\nUnset away message.");

	ADD_CMD ("away", away, "/away [msg]\tSet away message.\n/away -\tUnset.");

	ADD_CMD ("afk", afk, "/afk [msg]\tAdd afk message (default 'afk') to nick.\n/afk -\tUnset.");

	ADD_CMD ("nick", nick, "/nick <nickname>\nSet your nickname.");

	ADD_CMD ("topic", topic, "/topic [message]\nSets or get's the channel topic.");

	ADD_CMD ("msg", msg, "/msg [-n] <recipient> [message]\nSend a message to another user. '-n' suppresses new tab.");

	ADD_CMD ("ping", ping, "/ping [recipient]\nSend a CTCP ping to recipient. If in a private channel recipient can be omitted.");

	ADD_CMD ("amsg", amsg, "/amsg [message]\nSend a message to all users in channel.");

	ADD_CMD ("me", me, "/me <action>\nSend an 'Action' to the channel.");

	ADD_CMD ("tab", tab, "/tab <n>\nSwitch to the n'th tab.");

	ADD_CMD ("sort", sort, "/sort\nSort tab list into an intuitive order.");

	ADD_CMD ("left", left, "/left\nSwap current tab to the left.");

	ADD_CMD ("right", right, "/right\nSwap current tab to the right.");

	ADD_CMD ("ignore", ignore, "/ignore [-ipd] nick[!user[@host]]\n/ignore [-ipd] -r regex\n"
						"/ignore -l\n\t-i case insensitive\n\t-p ignore private messages\n"
						"\t-d remove rule\n\t-r match on regex\n\t-l list active rules");

	ADD_CMD ("mode", mode, "/mode [nick [{+|-}mode]]\nSet, unset, or query mode flags.");

	ADD_CMD ("grep", grep, "/grep [pattern]\nSearch scrollback for a pattern.");

	ADD_CMD ("cmd", cmd, "/cmd <command>\nSend raw command to the server.");
	ADD_CMD ("quote", cmd, "/quote <command>\nSend raw command to the server.");

	ADD_CMD ("help", help, "/help [command]\nGet usage information for a command or list information for all commands.");

	return (0);

}

#undef ADD_CMD

struct cmd_t cmd_ambig = {.name = NULL, .help = "Handler for ambiguous commands.  Not callable directly.", .func=_handle_ambig};

struct cmd_t *get_cmd(char *cmd)
{
	size_t len = strlen(cmd);
	int found = -1;
	for(unsigned int i = 0; i < ncmds; i++)
	{
		if (commands[i].name && strncmp(commands[i].name, cmd, len) == 0) {
			if (found < 0) {
				found = i;
			} else {
				add_to_buffer(cbuf, STA, NORMAL, 0, false, ": ambiguous command", cmd);
				return &cmd_ambig;
			}
		}
	}
	if (found < 0)
		return NULL;
	return &commands[found];
}

int call_cmd(char *cmd, char *args, char **qmsg, fd_set * master, int *fdmax)
{
	struct cmd_t *c = get_cmd(cmd);
	if (c)
		return c->func(cmd, args, qmsg, master, fdmax, 0);
	if (!cmd)
		cmd = "";
	char dstr[8 + strlen(cmd)];
	sprintf(dstr, "/%s: ", cmd);
	add_to_buffer (cbuf, ERR, NORMAL, 0, false, "Unrecognised command!", dstr);
	return 0;
}

//commands may not have args or use the original cmd 
//so we have to turn of warnings for them so that
//they don't get caught in -Werror -Russell
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

CMD_FUN(ambig)
{
	char *cmds;
	size_t cl, ci, len = strlen(cmd);
	init_char(&cmds, &cl, &ci);
	for(unsigned int i = 0; i < ncmds; i++)
	{
		if (!commands[i].name)
			continue;
		if (strncmp(commands[i].name, cmd, len))
			continue;
		if (ci)
			append_str(&cmds, &cl, &ci, ", ");
		if (commands[i].help && *commands[i].help)
		{
			append_str(&cmds, &cl, &ci, commands[i].name);
		}
		else
		{
			append_char(&cmds, &cl, &ci, '{');
			append_str(&cmds, &cl, &ci, commands[i].name);
			append_char(&cmds, &cl, &ci, '}');
		}
	}
	add_to_buffer(cbuf,STA,NORMAL,0,false,cmds,"[matches] ");
	free(cmds);
	return 0;
}

CMD_FUN (help)
{
	if(args)
	{
		struct cmd_t *c = get_cmd(args);
		if (c == NULL)
			add_to_buffer(cbuf,ERR,NORMAL,0,false,args,"/help: unrecognised command: ");
		else if (c == &cmd_ambig)
			return CMD_FNAME(ambig)(args, args, qmsg, master, fdmax, flag);
		else if (c->help && *c->help)
			add_to_buffer(cbuf,STA,NORMAL,0,false,c->help,"Usage: ");
		else
			add_to_buffer(cbuf,ERR,NORMAL,0,false,": No usage information.",c->name);
	}
	else
	{
		char *cmds;
		size_t cl, ci;
		init_char(&cmds, &cl, &ci);
		append_char(&cmds, &cl, &ci, '\n');
		for(unsigned int i = 0; i < ncmds; i++)
		{
			if (i)
				append_str(&cmds, &cl, &ci, ", ");
			if (commands[i].help && *commands[i].help)
			{
				append_str(&cmds, &cl, &ci, commands[i].name);
			}
			else
			{
				append_char(&cmds, &cl, &ci, '{');
				append_str(&cmds, &cl, &ci, commands[i].name);
				append_char(&cmds, &cl, &ci, '}');
			}
		}	
		add_to_buffer(cbuf,STA,NORMAL,0,false,cmds,"HELP: Commands");
		free(cmds);
	}
	return 0;
}

CMD_FUN (quit)
{
	if (args)
	{
		free (*qmsg);
		*qmsg = strdup (args);
	}
	add_to_buffer (cbuf, STA, NORMAL, 0, false, "Exited quirc",
		       "/quit: ");
	return (-1);
}

CMD_FUN (close)
{
	switch (bufs[cbuf].type)
	{
	case STATUS:
		CMD_FNAME(quit)("quit",args,qmsg,master,fdmax,0);
		break;
	case SERVER:
		if (bufs[cbuf].live)
		{
			CMD_FNAME(disconnect)("disconnect",args,qmsg,master,fdmax,0);
			return (0);
		}
		else
		{
			int b2;
			for (b2 = 1; b2 < nbufs; b2++)
			{
				while ((b2 < nbufs) && (bufs[b2].type != SERVER)
				       && (bufs[b2].server == cbuf))
				{
					bufs[b2].live = false;
					free_buffer (b2);
				}
			}
			free_buffer (cbuf);
			return (0);
		}
		break;
	case CHANNEL:
		if (bufs[cbuf].live)
		{
			CMD_FNAME(part)("part",args,qmsg,master,fdmax,0);
			return (0);
		}
		else
		{
			free_buffer (cbuf);
			return (0);
		}
		break;
	default:
		bufs[cbuf].live = false;
		free_buffer (cbuf);
		return (0);
		break;
	}
	return -1;
}

CMD_FUN (log)
{
	if (!args)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must specify a log type or /log -", "/log: ");
		return (0);
	}
	if (bufs[cbuf].logf)
	{
		fclose (bufs[cbuf].logf);
		bufs[cbuf].logf = NULL;
	}
	if (strcmp (args, "-") == 0)
	{
		add_to_buffer (cbuf, STA, QUIET, 0, false,
			       "Disabled logging of this buffer", "/log: ");
		return (0);
	}
	else
	{
		char *type = strtok (args, " ");
		if (type)
		{
			char *fn = strtok (NULL, "");
			if (fn)
			{
				logtype logt;
				if (strcasecmp (type, "plain") == 0)
					logt = LOGT_PLAIN;
				else if (strcasecmp (type, "symbolic") == 0)
					logt = LOGT_SYMBOLIC;
				else
				{
					add_to_buffer (cbuf, ERR, NORMAL, 0,
						       false,
						       "Unrecognised log type (valid types are: plain, symbolic)",
						       "/log: ");
					return (0);
				}
				FILE *fp = fopen (fn, "a");
				if (!fp)
				{
					add_to_buffer (cbuf, ERR, NORMAL, 0,
						       false,
						       "Failed to open log file for append",
						       "/log: ");
					add_to_buffer (cbuf, ERR, NORMAL, 0,
						       false,
						       strerror (errno),
						       "fopen: ");
					return (0);
				}
				log_init (fp, logt);
				bufs[cbuf].logf = fp;
				bufs[cbuf].logt = logt;
				add_to_buffer (cbuf, STA, QUIET, 0, false,
					       "Enabled logging of this buffer",
					       "/log: ");
				return (0);
			}
			else
			{
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Must specify a log file",
					       "/log: ");
				return (0);
			}
		}
		else
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Must specify a log type or /log -",
				       "/log: ");
			return (0);
		}
	}
}

CMD_FUN (set)
{
	bool osp = show_prefix, odbg = debug, oind = indent;
	unsigned int omln = maxnlen;
	if (args)
	{
		char *opt = strtok (args, " ");
		if (opt)
		{
			char *val = strtok (NULL, "");
#include "config_set.c"
		else if (strcmp (opt, "conf") == 0)
		{
			if (bufs[cbuf].type == CHANNEL)
			{
				if (val)
				{
					if (isdigit (*val))
					{
						unsigned int value;
						sscanf (val, "%u", &value);
						bufs[cbuf].conf = value;
					}
					else if (strcmp (val, "+") == 0)
					{
						bufs[cbuf].conf = true;
					}
					else if (strcmp (val, "-") == 0)
					{
						bufs[cbuf].conf = false;
					}
					else
					{
						add_to_buffer (cbuf, ERR,
							       NORMAL, 0,
							       false,
							       "option 'conf' is boolean, use only 0/1 or -/+ to set",
							       "/set: ");
					}
				}
				else
					bufs[cbuf].conf = true;
				if (bufs[cbuf].conf)
					add_to_buffer (cbuf, STA, QUIET, 0,
						       false,
						       "conference mode enabled for this channel",
						       "/set: ");
				else
					add_to_buffer (cbuf, STA, QUIET, 0,
						       false,
						       "conference mode disabled for this channel",
						       "/set: ");
				mark_buffer_dirty (cbuf);
				redraw_buffer ();
			}
			else
			{
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Not a channel!",
					       "/set conf: ");
			}
		}
		else if (strcmp (opt, "uname") == 0)
		{
			if (val)
			{
				free (username);
				username = strdup (val);
				add_to_buffer (cbuf, STA, QUIET, 0, false,
					       username, "/set uname ");
			}
			else
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Non-null value required for uname",
					       "/set uname: ");
		}
		else if (strcmp (opt, "fname") == 0)
		{
			if (val)
			{
				free (fname);
				fname = strdup (val);
				add_to_buffer (cbuf, STA, QUIET, 0, false,
					       fname, "/set fname ");
			}
			else
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Non-null value required for fname",
					       "/set fname: ");
		}
		else if (strcmp (opt, "pass") == 0)
		{
			if (val)
			{
				free (pass);
				pass = strdup (val);
				char *p = val;
				while (*p)
					*p++ = '*';
				add_to_buffer (cbuf, STA, QUIET, 0, false,
					       val, "/set pass ");
			}
			else
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Non-null value required for pass",
					       "/set pass: ");
		}
		else
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "No such option!", "/set: ");
		}
		}
		else
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "But what do you want to set?",
				       "/set: ");
		}
	}
	else
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "But what do you want to set?", "/set: ");
	}
	if ((show_prefix != osp) || (maxnlen != omln) || (indent != oind))
	{
		for (int b = 0; b < nbufs; b++)
			mark_buffer_dirty (b);
	}
	if (debug && !odbg)
	{
		push_ring (&d_buf, DEBUG);
	}
	else if (odbg && !debug)
	{
		init_ring (&d_buf);
		d_buf.loop = true;
	}
	return (0);
}

CMD_FUN (server)
{
	if (args)
	{
		char *server = args;
		char *newpass = strchr(server, ' ');
		if (newpass)
			*newpass++ = 0;
		else
			newpass = pass;
		char *newport = strchr (server, ':');
		if (newport)
			*newport++ = 0;
		else
			newport = portno;
		int b;
		for (b = 1; b < nbufs; b++)
		{
			if ((bufs[b].type == SERVER)
			    &&
			    (irc_strcasecmp
			     (server, bufs[b].bname,
			      bufs[b].casemapping) == 0))
			{
				if (bufs[b].live)
				{
					cbuf = b;
					redraw_buffer ();
					return (0);
				}
				else
				{
					cbuf = b;
					cmd = "reconnect";
					redraw_buffer ();
					break;
				}
			}
		}
		if (b >= nbufs)
		{
			char dstr[30 + strlen (server) + strlen (newport)];
			sprintf (dstr, "Connecting to %s on port %s...",
				 server, newport);
#if ASYNCH_NL
			__attribute__ ((unused)) int *p = fdmax;
			nl_list *nl = irc_connect (server, newport);
			if (nl)
			{
				if (newpass)
				{
					nl->autoent=malloc(sizeof(servlist));
					nl->autoent->name=NULL;
					nl->autoent->portno=NULL;
					nl->autoent->nick=NULL;
					nl->autoent->pass=strdup(newpass);
					nl->autoent->chans=NULL;
					nl->autoent->next=NULL;
					nl->autoent->igns=NULL;
				}
				nl->reconn_b = 0;
				add_to_buffer (0, STA, QUIET, 0, false, dstr,
					       "/server: ");
				redraw_buffer ();
			}
#else
			int serverhandle =
				irc_connect (server, newport, master, fdmax);
			if (serverhandle)
			{
				bufs = (buffer *) realloc (bufs,
							   ++nbufs *
							   sizeof (buffer));
				init_buffer (nbufs - 1, SERVER, server,
					     buflines);
				cbuf = nbufs - 1;
				bufs[cbuf].handle = serverhandle;
				bufs[cbuf].nick =
					bufs[0].nick ? strdup (bufs[0].
							       nick) : NULL;
				bufs[cbuf].key = strdup(newpass);
				bufs[cbuf].server = cbuf;
				bufs[cbuf].conninpr = true;
				add_to_buffer (cbuf, STA, QUIET, 0, false,
					       dstr, "/server: ");
				redraw_buffer ();
			}
#endif
		}
	}
	else
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must specify a server!", "/server: ");
	}
	return (0);
}

CMD_FUN (reconnect)
{
	if (bufs[cbuf].server)
	{
		if (!SERVER (cbuf).live)
		{
			char *newport;
			if (args)
			{
				newport = args;
			}
			else if (SERVER (cbuf).autoent
				 && SERVER (cbuf).autoent->portno)
			{
				newport = SERVER (cbuf).autoent->portno;
			}
			else
			{
				newport = portno;
			}
			char dstr[30 + strlen (SERVER (cbuf).serverloc) +
				  strlen (newport)];
			sprintf (dstr, "Connecting to %s on port %s...",
				 SERVER (cbuf).serverloc, newport);
#if ASYNCH_NL
			nl_list *nl =
				irc_connect (SERVER (cbuf).serverloc,
					     newport);
			if (nl)
			{
				nl->reconn_b = bufs[cbuf].server;
				add_to_buffer (bufs[cbuf].server, STA, QUIET,
					       0, false, dstr, "/server: ");
				redraw_buffer ();
			}
			else
			{
				add_to_buffer (bufs[cbuf].server, ERR, NORMAL,
					       0, false,
					       "malloc failure (see status)",
					       "/server: ");
				redraw_buffer ();
			}
#else /* ASYNCH_NL */
			int serverhandle =
				irc_connect (SERVER (cbuf).serverloc, newport,
					     master, fdmax);
			if (serverhandle)
			{
				int b = bufs[cbuf].server;
				bufs[b].handle = serverhandle;
				int b2;
				for (b2 = 1; b2 < nbufs; b2++)
				{
					if (bufs[b2].server == b)
						bufs[b2].handle =
							serverhandle;
				}
				bufs[cbuf].conninpr = true;
				free (bufs[cbuf].realsname);
				bufs[cbuf].realsname = NULL;
				add_to_buffer (cbuf, STA, QUIET, 0, false,
					       dstr, "/server: ");
			}
#endif /* ASYNCH_NL */
		}
		else
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Already connected to server",
				       "/reconnect: ");
		}
	}
	else
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must be run in the context of a server!",
			       "/reconnect: ");
	}
	return (0);
}

CMD_FUN (disconnect)
{
	int b = bufs[cbuf].server;
	if (b > 0)
	{
		if (bufs[b].handle)
		{
			if (bufs[b].live)
			{
				char quit[7 +
					  strlen (args ? args
						  : (qmsg
						     && *qmsg) ? *qmsg :
						  "quIRC quit")];
				sprintf (quit, "QUIT %s",
					 args ? args : (qmsg
							&& *qmsg) ? *qmsg :
					 "quIRC quit");
				irc_tx (bufs[b].handle, quit);
			}
			close (bufs[b].handle);
			FD_CLR (bufs[b].handle, master);
		}
		int b2;
		for (b2 = 1; b2 < nbufs; b2++)
		{
			while ((b2 < nbufs) && (bufs[b2].type != SERVER)
			       && ((bufs[b2].server == b)
				   || (bufs[b2].server == 0)))
			{
				bufs[b2].live = false;
				free_buffer (b2);
			}
		}
		bufs[b].live = false;
		free_buffer (b);
		cbuf = 0;
		redraw_buffer ();
	}
	else
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Can't disconnect (status)!", "/disconnect: ");
	}
	return (0);
}

CMD_FUN (realsname)
{
	int b = bufs[cbuf].server;
	if (b > 0)
	{
		if (bufs[b].realsname)
			add_to_buffer (cbuf, STA, NORMAL, 0, false,
				       bufs[b].realsname, "/realsname: ");
		else
			add_to_buffer (cbuf, ERR, NORMAL, 0, false, "unknown",
				       "/realsname ");
	}
	else
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "(status) is not a server", "/realsname: ");
	return (0);
}

CMD_FUN (join)
{
	if (!SERVER (cbuf).handle)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must be run in the context of a server!",
			       "/join: ");
	}
	else if (!SERVER (cbuf).live)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Disconnected, can't send", "/join: ");
	}
	else if (args)
	{
		char *chan = strtok (args, " ");
		if (!chan)
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Must specify a channel!", "/join: ");
		}
		else
		{
			char *pass = strtok (NULL, ", ");
			servlist *serv = SERVER (cbuf).autoent;
			if (!serv)
			{
				serv = SERVER (cbuf).autoent =
					malloc (sizeof (servlist));
				serv->name = NULL;
				serv->portno = NULL;
				serv->nick = NULL;
				serv->pass = NULL;
				serv->chans = NULL;
				serv->next = NULL;
				serv->igns = NULL;
			}
			if (pass)
			{
				chanlist *curr = malloc (sizeof (chanlist));
				if (curr)
				{
					curr->name = strdup (chan);
					curr->key = strdup (pass);
					curr->next = serv->chans;
					serv->chans = curr;
				}
			}
			else
			{
				chanlist *curr = serv->chans;
				while (curr)
				{
					if (irc_strcasecmp
					    (curr->name, chan,
					     SERVER (cbuf).casemapping) == 0)
					{
						pass = curr->key;
						break;
					}
					curr = curr->next;
				}
			}
			if (!pass)
				pass = "";
			char joinmsg[8 + strlen (chan) + strlen (pass)];
			sprintf (joinmsg, "JOIN %s %s", chan, pass);
			irc_tx (SERVER (cbuf).handle, joinmsg);
		}
	}
	else
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must specify a channel!", "/join: ");
	}
	return (0);
}

CMD_FUN (rejoin)
{
	if (bufs[cbuf].type == PRIVATE)
	{
		if (!(SERVER (cbuf).handle && SERVER (cbuf).live))
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Disconnected, can't send",
				       "/rejoin: ");
		}
		else if (bufs[cbuf].live)
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Already in this channel",
				       "/rejoin: ");
		}
		else
		{
			bufs[cbuf].live = true;
		}
	}
	else if (bufs[cbuf].type != CHANNEL)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "View is not a channel!", "/rejoin: ");
	}
	else if (!(SERVER (cbuf).handle && SERVER (cbuf).live))
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Disconnected, can't send", "/rejoin: ");
	}
	else if (bufs[cbuf].live)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Already in this channel", "/rejoin: ");
	}
	else
	{
		char *chan = bufs[cbuf].bname;
		char *pass = args;
		if (pass)
			bufs[cbuf].lastkey = strdup (pass);
		else
			pass = bufs[cbuf].key;
		if (!pass)
			pass = "";
		char joinmsg[8 + strlen (chan) + strlen (pass)];
		sprintf (joinmsg, "JOIN %s %s", chan, pass);
		irc_tx (SERVER (cbuf).handle, joinmsg);
		redraw_buffer ();
	}
	return (0);
}

CMD_FUN (part)
{
	if (bufs[cbuf].type != CHANNEL)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "This view is not a channel!", "/part: ");
	}
	else
	{
		if (LIVE (cbuf) && SERVER (cbuf).handle)
		{
			char partmsg[8 + strlen (bufs[cbuf].bname)];
			sprintf (partmsg, "PART %s", bufs[cbuf].bname);
			irc_tx (SERVER (cbuf).handle, partmsg);
			add_to_buffer (cbuf, PART, NORMAL, 0, true, "Leaving",
				       "/part: ");
		}
		// when you try to /part a dead tab, interpret it as a /close
		int parent = bufs[cbuf].server;
		bufs[cbuf].live = false;
		free_buffer (cbuf);
		cbuf = parent;
		redraw_buffer ();
	}
	return (0);
}

CMD_FUN (unaway)
{
	return CMD_FNAME (away) ("away", "-", qmsg, master, fdmax, 0);
}

CMD_FUN (away)
{
	const char *am =
		"Gone away, gone away, was it one of you took it away?";
	if (args)
		am = args;
	if (SERVER (cbuf).handle)
	{
		if (LIVE (cbuf))
		{
			if (strcmp (am, "-"))
			{
				char nmsg[8 + strlen (am)];
				sprintf (nmsg, "AWAY :%s", am);
				irc_tx (SERVER (cbuf).handle, nmsg);
			}
			else
			{
				irc_tx (SERVER (cbuf).handle, "AWAY");	// unmark away
			}
		}
		else
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Tab not live, can't send", "/away: ");
		}
	}
	else
	{
		if (cbuf)
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Tab not live, can't send", "/away: ");
		else
		{
			int b;
			for (b = 0; b < nbufs; b++)
			{
				if (bufs[b].type == SERVER)
				{
					if (bufs[b].handle)
					{
						if (LIVE (b))
						{
							if (strcmp (am, "-"))
							{
								char nmsg[8 +
									  strlen
									  (am)];
								sprintf (nmsg,
									 "AWAY :%s",
									 am);
								irc_tx (bufs
									[b].
									handle,
									nmsg);
							}
							else
							{
								irc_tx (bufs[b].handle, "AWAY");	// unmark away
							}
						}
						else
							add_to_buffer (b, ERR,
								       NORMAL,
								       0,
								       false,
								       "Tab not live, can't send",
								       "/away: ");
					}
					else
						add_to_buffer (b, ERR, NORMAL,
							       0, false,
							       "Tab not live, can't send",
							       "/away: ");
				}
			}
		}
	}
	return (0);
}

CMD_FUN (afk)
{
	int aalloc = false;
	const char *afm = "afk";
	if (args)
		afm = strtok (args, " ");
	const char *p = SERVER (cbuf).nick;
	int n = strcspn (p, "|");
	char *nargs = malloc (n + strlen (afm) + 2);
	if (nargs)
	{
		cmd = "nick";
		strncpy (nargs, p, n);
		nargs[n] = 0;
		if (strcmp (afm, "-"))
		{
			strcat (nargs, "|");
			strcat (nargs, afm);
		}
		args = nargs;
		aalloc = true;
	}
	return CMD_FNAME (nick) (cmd, args, qmsg, master, fdmax, aalloc);

}

CMD_FUN (nick)
{
	if (args)
	{
		char *nn = strtok (args, " ");
		if (SERVER (cbuf).handle)
		{
			if (LIVE (cbuf))
			{
				free (SERVER (cbuf).nick);
				SERVER (cbuf).nick = strdup (nn);
				char nmsg[8 + strlen (SERVER (cbuf).nick)];
				sprintf (nmsg, "NICK %s", SERVER (cbuf).nick);
				irc_tx (SERVER (cbuf).handle, nmsg);
				add_to_buffer (cbuf, STA, QUIET, 0, false,
					       "Changing nick", "/nick: ");
			}
			else
			{
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Tab not live, can't send",
					       "/nick: ");
			}
		}
		else
		{
			if (cbuf)
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Tab not live, can't send",
					       "/nick: ");
			else
			{
				free (bufs[0].nick);
				bufs[0].nick = strdup (nn);
				defnick = false;
				add_to_buffer (cbuf, STA, QUIET, 0, false,
					       "Default nick changed",
					       "/nick: ");
			}
		}
	}
	else
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must specify a nickname!", "/nick: ");
	}
	if (flag)
		free (args);
	return (0);
}

CMD_FUN (topic)
{
	if (args)
	{
		if (bufs[cbuf].type == CHANNEL)
		{
			if (SERVER (cbuf).handle)
			{
				if (LIVE (cbuf))
				{
					char tmsg[10 +
						  strlen (bufs[cbuf].bname) +
						  strlen (args)];
					sprintf (tmsg, "TOPIC %s :%s",
						 bufs[cbuf].bname, args);
					irc_tx (SERVER (cbuf).handle, tmsg);
				}
				else
				{
					add_to_buffer (cbuf, ERR, NORMAL, 0,
						       false,
						       "Tab not live, can't send",
						       "/topic: ");
				}
			}
			else
			{
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Can't send to channel - not connected!",
					       "/topic: ");
			}
		}
		else
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Can't set topic - view is not a channel!",
				       "/topic: ");
		}
	}
	else
	{
		if (bufs[cbuf].type == CHANNEL)
		{
			if (SERVER (cbuf).handle)
			{
				if (LIVE (cbuf))
				{
					char tmsg[8 +
						  strlen (bufs[cbuf].bname)];
					sprintf (tmsg, "TOPIC %s",
						 bufs[cbuf].bname);
					irc_tx (SERVER (cbuf).handle, tmsg);
				}
				else
				{
					add_to_buffer (cbuf, ERR, NORMAL, 0,
						       false,
						       "Tab not live, can't send",
						       "/topic: ");
				}
			}
			else
			{
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Can't send to channel - not connected!",
					       "/topic: ");
			}
		}
		else
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Can't get topic - view is not a channel!",
				       "/topic: ");
		}
	}
	return (0);
}

CMD_FUN (msg)
{
	if (!SERVER (cbuf).handle)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must be run in the context of a server!",
			       "/msg: ");
	}
	else if (args)
	{
		bool no_tab = false;
		char *dest = strtok (args, " ");
		if (strcmp (dest, "-n") == 0)
		{
			no_tab = true;
			dest = strtok (NULL, " ");
		}
		if (!dest)
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Must specify a recipient!", "/msg: ");
			return (0);
		}
		char *text = strtok (NULL, "");
		if (!no_tab)
		{
			int b2 = makeptab (bufs[cbuf].server, dest);
			cbuf = b2;
			redraw_buffer ();
		}
		if (text)
		{
			if (SERVER (cbuf).handle)
			{
				if (LIVE (cbuf))
				{
					char privmsg[12 + strlen (dest) +
						     strlen (text)];
					sprintf (privmsg, "PRIVMSG %s :%s",
						 dest, text);
					irc_tx (SERVER (cbuf).handle,
						privmsg);
					ctcp_strip (text, SERVER (cbuf).nick,
						    cbuf, false, false, true,
						    true);
					if (no_tab)
					{
						add_to_buffer (cbuf, STA,
							       QUIET, 0,
							       false, "sent",
							       "/msg -n: ");
					}
					else
					{
						while (text[strlen (text) - 1]
						       == '\n')
							text[strlen (text) - 1] = 0;	// stomp out trailing newlines, they break things
						add_to_buffer (cbuf, MSG,
							       NORMAL, 0,
							       true, text,
							       SERVER (cbuf).
							       nick);
					}
				}
				else
				{
					add_to_buffer (cbuf, ERR, NORMAL, 0,
						       false,
						       "Tab not live, can't send",
						       "/msg: ");
				}
			}
			else
			{
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Can't send to channel - not connected!",
					       "/msg: ");
			}
		}
	}
	else
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must specify a recipient!", "/msg: ");
	}
	return (0);
}

CMD_FUN (ping)
{
	if (!SERVER (cbuf).handle)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must be run in the context of a server!",
			       "/ping: ");
	}
	else
	{
		const char *dest = NULL;
		if (args)
			dest = strtok (args, " ");
		if (!dest && bufs[cbuf].type == PRIVATE)
			dest = bufs[cbuf].bname;
		if (dest)
		{
			if (SERVER (cbuf).handle)
			{
				if (LIVE (cbuf))
				{
					struct timeval tv;
					gettimeofday (&tv, NULL);
					char privmsg[64 + strlen (dest)];
					snprintf (privmsg, 64 + strlen (dest),
						  "PRIVMSG %s :\001PING %u %u\001",
						  dest,
						  (unsigned int) tv.tv_sec,
						  (unsigned int) tv.tv_usec);
					irc_tx (SERVER (cbuf).handle,
						privmsg);
				}
				else
				{
					add_to_buffer (cbuf, ERR, NORMAL, 0,
						       false,
						       "Tab not live, can't send",
						       "/ping: ");
				}
			}
			else
			{
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Can't send - not connected!",
					       "/ping: ");
			}
		}
		else
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Must specify a recipient!",
				       "/ping: ");
		}
	}
	return (0);
}

CMD_FUN (amsg)
{
	if (!bufs[cbuf].server)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must be run in the context of a server!",
			       "/amsg: ");
	}
	else if (args)
	{
		int b2;
		for (b2 = 1; b2 < nbufs; b2++)
		{
			if ((bufs[b2].server == bufs[cbuf].server)
			    && (bufs[b2].type == CHANNEL))
			{
				if (LIVE (b2))
				{
					char privmsg[12 +
						     strlen (bufs[b2].bname) +
						     strlen (args)];
					sprintf (privmsg, "PRIVMSG %s :%s",
						 bufs[b2].bname, args);
					irc_tx (bufs[b2].handle, privmsg);
					while (args[strlen (args) - 1] ==
					       '\n')
						args[strlen (args) - 1] = 0;	// stomp out trailing newlines, they break things
					bool al = bufs[b2].alert;	// save alert status...
					int hi = bufs[b2].hi_alert;
					add_to_buffer (b2, MSG, NORMAL, 0,
						       true, args,
						       SERVER (b2).nick);
					bufs[b2].alert = al;	// and restore it
					bufs[b2].hi_alert = hi;
				}
				else
				{
					add_to_buffer (b2, ERR, NORMAL, 0,
						       false,
						       "Tab not live, can't send",
						       "/amsg: ");
				}
			}
		}
	}
	else
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must specify a message!", "/amsg: ");
	}
	return (0);
}

CMD_FUN (me)
{
	if (!((bufs[cbuf].type == CHANNEL) || (bufs[cbuf].type == PRIVATE)))
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Can't talk here, not a channel/private chat",
			       "/me: ");
	}
	else if (args)
	{
		if (SERVER (cbuf).handle)
		{
			if (LIVE (cbuf))
			{
				char privmsg[32 + strlen (bufs[cbuf].bname) +
					     strlen (args)];
				sprintf (privmsg,
					 "PRIVMSG %s :\001ACTION %s\001",
					 bufs[cbuf].bname, args);
				irc_tx (SERVER (cbuf).handle, privmsg);
				while (args[strlen (args) - 1] == '\n')
					args[strlen (args) - 1] = 0;	// stomp out trailing newlines, they break things
				add_to_buffer (cbuf, ACT, NORMAL, 0, true,
					       args, SERVER (cbuf).nick);
			}
			else
			{
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Tab not live, can't send",
					       "/me: ");
			}
		}
		else
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Can't send to channel - not connected!",
				       "/msg: ");
		}
	}
	else
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must specify an action!", "/me: ");
	}
	return (0);
}

CMD_FUN (tab)
{
	if (!args)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must specify a tab!", "/tab: ");
	}
	else
	{
		int bufn;
		if (sscanf (args, "%d", &bufn) == 1)
		{
			if ((bufn >= 0) && (bufn < nbufs))
			{
				cbuf = bufn;
				redraw_buffer ();
			}
			else
			{
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "No such tab!", "/tab: ");
			}
		}
		else
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Must specify a tab!", "/tab: ");
		}
	}
	return (0);
}

CMD_FUN (sort)
{
	int newbufs[nbufs];
	buffer bi[nbufs];
	newbufs[0] = 0;
	int b, buf = 1;
	for (b = 0; b < nbufs; b++)
	{
		bi[b] = bufs[b];
		if (bufs[b].type == SERVER)
		{
			newbufs[buf++] = b;
			int b2;
			for (b2 = 1; b2 < nbufs; b2++)
			{
				if ((bufs[b2].server == b) && (b2 != b))
				{
					newbufs[buf++] = b2;
				}
			}
		}
	}
	if (buf != nbufs)
	{
		add_to_buffer (cbuf, ERR, QUIET, 0, false,
			       "Internal error (bad count)", "/sort: ");
		return (0);
	}
	int serv = 0, cb = 0;
	for (b = 0; b < nbufs; b++)
	{
		bufs[b] = bi[newbufs[b]];
		if (bufs[b].type == SERVER)
			serv = b;
		bufs[b].server = serv;
		if (newbufs[b] == cbuf)
			cb = b;
	}
	cbuf = cb;
	redraw_buffer ();
	return (0);
}

CMD_FUN (left)
{
	if (cbuf < 2)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Can't move (status) tab!", "/left: ");
	}
	else
	{
		buffer tmp = bufs[cbuf];
		bufs[cbuf] = bufs[cbuf - 1];
		bufs[cbuf - 1] = tmp;
		cbuf--;
		int i;
		for (i = 0; i < nbufs; i++)
		{
			if (bufs[i].server == cbuf)
			{
				bufs[i].server++;
			}
			else if (bufs[i].server == cbuf + 1)
			{
				bufs[i].server--;
			}
		}
		redraw_buffer ();
	}
	return (0);
}

CMD_FUN (right)
{
	if (!cbuf)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Can't move (status) tab!", "/right: ");
	}
	else if (cbuf == nbufs - 1)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Nowhere to move to!", "/right: ");
	}
	else
	{
		buffer tmp = bufs[cbuf];
		bufs[cbuf] = bufs[cbuf + 1];
		bufs[cbuf + 1] = tmp;
		cbuf++;
		int i;
		for (i = 0; i < nbufs; i++)
		{
			if (bufs[i].server == cbuf - 1)
			{
				bufs[i].server++;
			}
			else if (bufs[i].server == cbuf)
			{
				bufs[i].server--;
			}
		}
		redraw_buffer ();
	}
	return (0);
}

CMD_FUN (ignore)
{
	if (!args)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Missing arguments!", "/ignore: ");
	}
	else
	{
		char *arg = strtok (args, " ");
		bool regex = false;
		bool icase = false;
		bool pms = false;
		bool del = false;
		while (arg)
		{
			if (*arg == '-')
			{
				if (strcmp (arg, "-i") == 0)
				{
					icase = true;
				}
				else if (strcmp (arg, "-r") == 0)
				{
					regex = true;
				}
				else if (strcmp (arg, "-d") == 0)
				{
					del = true;
				}
				else if (strcmp (arg, "-p") == 0)
				{
					pms = true;
				}
				else if (strcmp (arg, "-l") == 0)
				{
					i_list ();
					break;
				}
				else if (strcmp (arg, "--") == 0)
				{
					arg = strtok (NULL, "");
					continue;
				}
			}
			else
			{
				if (del)
				{
					if (i_cull (&bufs[cbuf].ilist, arg))
					{
						add_to_buffer (cbuf, STA,
							       QUIET, 0,
							       false,
							       "Entries deleted",
							       "/ignore -d: ");
					}
					else
					{
						add_to_buffer (cbuf, ERR,
							       NORMAL, 0,
							       false,
							       "No entries deleted",
							       "/ignore -d: ");
					}
				}
				else if (regex)
				{
					name *new =
						n_add (&bufs[cbuf].ilist, arg,
						       bufs[cbuf].
						       casemapping);
					if (new)
					{
						add_to_buffer (cbuf, STA,
							       QUIET, 0,
							       false,
							       "Entry added",
							       "/ignore: ");
						new->icase = icase;
						new->pms = pms;
					}
				}
				else
				{
					char *isrc, *iusr, *ihst;
					prefix_split (arg, &isrc, &iusr,
						      &ihst);
					if ((!isrc) || (*isrc == 0)
					    || (*isrc == '*'))
						isrc = "[^!@]*";
					if ((!iusr) || (*iusr == 0)
					    || (*iusr == '*'))
						iusr = "[^!@]*";
					if ((!ihst) || (*ihst == 0)
					    || (*ihst == '*'))
						ihst = "[^@]*";
					char expr[16 + strlen (isrc) +
						  strlen (iusr) +
						  strlen (ihst)];
					sprintf (expr, "^%s[_~]*!%s@%s$",
						 isrc, iusr, ihst);
					name *new =
						n_add (&bufs[cbuf].ilist,
						       expr,
						       bufs[cbuf].
						       casemapping);
					if (new)
					{
						add_to_buffer (cbuf, STA,
							       QUIET, 0,
							       false,
							       "Entry added",
							       "/ignore: ");
						new->icase = icase;
						new->pms = pms;
					}
				}
				break;
			}
			arg = strtok (NULL, " ");
		}
	}
	return (0);
}

CMD_FUN (mode)
{
	if (!SERVER (cbuf).handle)
	{
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
			       "Must be run in the context of a server!",
			       "/mode: ");
	}
	else
	{
		// /mode [{<user>|<banmask>|<limit>} [{\+|-}[[:alpha:]]+]]
		if (args)
		{
			char *user = strtok (args, " ");
			char *modes = strtok (NULL, "");
			if (modes)
			{
				if (LIVE (cbuf))
				{
					if (bufs[cbuf].type == CHANNEL)
					{
						char mmsg[8 +
							  strlen (bufs[cbuf].
								  bname) +
							  strlen (modes) +
							  strlen (user)];
						sprintf (mmsg,
							 "MODE %s %s %s",
							 bufs[cbuf].bname,
							 modes, user);
						irc_tx (SERVER (cbuf).handle,
							mmsg);
					}
					else
						add_to_buffer (cbuf, ERR,
							       NORMAL, 0,
							       false,
							       "This is not a channel",
							       "/mode: ");
				}
				else
					add_to_buffer (cbuf, ERR, NORMAL, 0,
						       false,
						       "Tab not live, can't send",
						       "/mode: ");
			}
			else
			{
				if (bufs[cbuf].type == CHANNEL)
				{
					name *curr = bufs[cbuf].nlist;
					while (curr)
					{
						if (irc_strcasecmp
						    (curr->data, user,
						     SERVER (cbuf).
						     casemapping) == 0)
						{
							if (curr ==
							    bufs[cbuf].us)
								goto youmode;
							char mm[12 +
								strlen (curr->
									data)
								+ curr->npfx];
							int mpos = 0;
							sprintf (mm,
								 "%s has mode %n-",
								 curr->data,
								 &mpos);
							if (mpos)
							{
								for (unsigned
								     int i =
								     0;
								     i <
								     curr->
								     npfx;
								     i++)
									mm[mpos++] = curr->prefixes[i].letter;
								if (curr->
								    npfx)
									mm[mpos] = 0;
								add_to_buffer
									(cbuf,
									 MODE,
									 NORMAL,
									 0,
									 false,
									 mm,
									 "/mode: ");
							}
							else
								add_to_buffer
									(cbuf,
									 ERR,
									 NORMAL,
									 0,
									 false,
									 "\"Impossible\" error (mpos==0)",
									 "/mode: ");
							break;
						}
						curr = curr->next;
					}
					if (!curr)
					{
						char mm[16 + strlen (user)];
						sprintf (mm,
							 "No such nick: %s",
							 user);
						add_to_buffer (cbuf, ERR,
							       NORMAL, 0,
							       false, mm,
							       "/mode: ");
					}
				}
				else
					add_to_buffer (cbuf, ERR, NORMAL, 0,
						       false,
						       "This is not a channel",
						       "/mode: ");
			}
		}
		else
		{
		      youmode:
			if (bufs[cbuf].type == CHANNEL)
			{
				if (bufs[cbuf].us)
				{
					char mm[20 +
						strlen (SERVER (cbuf).nick) +
						bufs[cbuf].us->npfx];
					int mpos = 0;
					sprintf (mm, "You (%s) have mode %n-",
						 SERVER (cbuf).nick, &mpos);
					if (mpos)
					{
						for (unsigned int i = 0;
						     i < bufs[cbuf].us->npfx;
						     i++)
							mm[mpos++] =
								bufs[cbuf].
								us->
								prefixes[i].
								letter;
						if (bufs[cbuf].us->npfx)
							mm[mpos] = 0;
						add_to_buffer (cbuf, MODE,
							       NORMAL, 0,
							       true, mm,
							       "/mode: ");
					}
					else
						add_to_buffer (cbuf, ERR,
							       NORMAL, 0,
							       false,
							       "\"Impossible\" error (mpos==0)",
							       "/mode: ");
				}
				else
					add_to_buffer (cbuf, ERR, NORMAL, 0,
						       false,
						       "\"Impossible\" error (us==NULL)",
						       "/mode: ");
			}
			else
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "This is not a channel",
					       "/mode: ");
		}
	}
	return (0);
}

CMD_FUN (grep)
{
	if (!args)
		args = SERVER(cbuf).nick;
	char errbuf[80];
	regex_t preg;
	int rc;

	rc = regcomp(&preg, args, REG_NOSUB | REG_EXTENDED);
	if (rc)
	{
		regerror(rc, &preg, errbuf, 80);
		add_to_buffer (cbuf, ERR, NORMAL, 0, false, errbuf,
			       "/grep: error: ");
	}
	else
	{
		int uline=bufs[cbuf].scroll-1;
		while(true)
		{
			if(--uline<0)
				uline+=bufs[cbuf].nlines;
			if ((uline==bufs[cbuf].ptr) || (!bufs[cbuf].filled && uline>bufs[cbuf].ptr))
			{
				add_to_buffer (cbuf, STA, NORMAL, 0, false, "No match",
					       "/grep: ");
				break;
			}
			if (regexec(&preg, bufs[cbuf].lt[uline], 0, NULL, 0)==0)
			{
				bufs[cbuf].scroll=(uline+1)%bufs[cbuf].nlines;
				bufs[cbuf].ascroll=0;
				redraw_buffer();
				break;
			}
		}
	}
	regfree(&preg);
	return (0);
}

CMD_FUN (cmd)
{
	if (args)
	{
		if (!SERVER (cbuf).handle)
		{
			add_to_buffer (cbuf, ERR, NORMAL, 0, false,
				       "Must be run in the context of a server!",
				       "/cmd: ");
		}
		else
		{
			bool force = false;
			if (strncmp (args, "-f", 2) == 0)
			{
				force = true;
				args++;
				while (*++args == ' ');
			}
			if (force || LIVE (cbuf))
			{
				irc_tx (SERVER (cbuf).handle, args);
				add_to_buffer (cbuf, STA, NORMAL, 0, false,
					       args, "/cmd: ");
			}
			else
			{
				add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "Tab not live, can't send",
					       "/cmd: ");
			}
		}
	}
	else
	{
		
		add_to_buffer (cbuf, ERR, NORMAL, 0, false,
					       "No command given.",
					       "/cmd: ");
	}
	return (0);
}

#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
