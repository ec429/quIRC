/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	config: rc file and option parsing
*/

#include "config.h"
#include "bits.h"
#include "colour.h"
#include "text.h"
#include "version.h"
#include "buffer.h"
#include "strbuf.h"
#include "ttyesc.h"

#include "keymap.c"

#include "config_globals.c"
bool autojoin;
char *username, *fname, *nick, *pass, *portno;
bool defnick;
servlist *servs;
name *igns;
char *version;
unsigned int nkeys;
keymod *kmap;

void loadkeys(FILE *fp)
{
	if(!fp) return;
	while(!feof(fp))
	{
		char *line=fgetl(fp);
		if(line)
		{
			if((*line)&&(*line!='#'))
			{
				keymod new;
				new.name=strtok(line, " \t");
				char *mod=strtok(NULL, " \t");
				if(!mod)
				{
					char msg[32+strlen(new.name)];
					sprintf(msg, "keys: missing mod in %s", new.name);
					atr_failsafe(&s_buf, ERR, msg, "init: ");
					free(line);
					continue;
				}
				off_t o=strlen(mod);
				if(o&1)
				{
					char msg[32+strlen(new.name)];
					sprintf(msg, "keys: odd mod length in %s", new.name);
					atr_failsafe(&s_buf, ERR, msg, "init: ");
					free(line);
					continue;
				}
				new.mod=malloc((o>>1)+1);
				if(!new.mod)
				{
					char msg[32+strlen(new.name)];
					sprintf(msg, "keys: malloc failure in %s", new.name);
					atr_failsafe(&s_buf, ERR, msg, "init: ");
					free(line);
					continue;
				}
				bool cont=false;
				for(int i=0;i<o;i+=2)
				{
					if(!(isxdigit(mod[i])&&isxdigit(mod[i+1])))
					{
						char msg[32+strlen(new.name)];
						sprintf(msg, "keys: bad mod (not hex) in %s", new.name);
						atr_failsafe(&s_buf, ERR, msg, "init: ");
						free(line);
						cont=true;
						break;
					}
					char buf[3];buf[0]=mod[i];buf[1]=mod[i+1];buf[2]=0;
					unsigned int c;
					if(sscanf(buf, "%x", &c)!=1)
					{
						char msg[32+strlen(new.name)];
						sprintf(msg, "keys: sscanf failed in %s", new.name);
						atr_failsafe(&s_buf, ERR, msg, "init: ");
						free(line);
						cont=true;
						break;
					}
					new.mod[i>>1]=c;
				}
				if(cont) continue;
				new.mod[o>>1]=0;
				bool match=false;
				for(unsigned int i=0;i<nkeys;i++)
				{
					if(strcmp(new.name, kmap[i].name)==0)
					{
						free(kmap[i].mod);
						kmap[i].mod=strdup(new.mod);
						match=true;
					}
				}
				if(!match)
				{
					char msg[32+strlen(new.name)];
					sprintf(msg, "keys: unrecognised name %s", new.name);
					atr_failsafe(&s_buf, ERR, msg, "init: ");
				}
				free(new.mod);
			}
		free(line);
		}
		else
			break;
	}
	return;
}

#include "config_check.c"

int def_config(void)
{
#include "config_def.c"
	autojoin=true;
	int l, c;
	if(!termsize(STDIN_FILENO, &c, &l))
	{
		height=max(l, 5);
		width=max(c, 30);
	}
	char *cols=getenv("COLUMNS"), *rows=getenv("LINES");
	if(cols) sscanf(cols, "%u", &width);
	if(rows) sscanf(rows, "%u", &height);
	servs=NULL;
	igns=NULL;
	portno=strdup("6667");
	char *eu=getenv("USER");
	username=strdup(eu?eu:"quirc");
	fname=malloc(64+strlen(VERSION_TXT));
	nick=strdup(eu?eu:"ac");
	defnick=true;
	pass=NULL;
	snprintf(fname, 64+strlen(VERSION_TXT), "quIRC %hhu.%hhu.%hhu%s%s : %s", VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_TXT[0]?"-":"", VERSION_TXT, CLIENT_SOURCE);
	version=malloc(16+strlen(VERSION_TXT));
	snprintf(version, 16+strlen(VERSION_TXT), "%hhu.%hhu.%hhu%s%s", VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_TXT[0]?"-":"", VERSION_TXT);
	return(0);
}

int rcread(FILE *rcfp)
{
	int nerrors=0; // number of lines with errors
	while(!feof(rcfp))
	{
		char *line=fgetl(rcfp);
		char *cmd=*line?strtok(line, " \t"):line;
		if((*cmd=='#') || (!*cmd)) // #lines are comments, blank lines are ignored
		{
			// do nothing
		}
		else if(*cmd=='%')
		{
			int which=0;
			// custom colours
			if(*++cmd=='S')
			{
				which=1;
				cmd++;
			}
			else if(*cmd=='R')
			{
				which=2;
				cmd++;
			}
			colour *what=NULL;
			if(strcmp(cmd, "msg")==0)
			{
				what=c_msg;
			}
			else if(strcmp(cmd, "notice")==0)
			{
				what=c_notice;
			}
			else if(strcmp(cmd, "join")==0)
			{
				what=c_join;
			}
			else if(strcmp(cmd, "part")==0)
			{
				what=c_part;
			}
			else if(strcmp(cmd, "quit")==0)
			{
				what=c_quit;
			}
			else if(strcmp(cmd, "nick")==0)
			{
				what=c_nick;
			}
			else if(strcmp(cmd, "act")==0)
			{
				what=c_actn;
			}
			else if(strcmp(cmd, "status")==0)
			{
				what=&c_status;which=-1;
			}
			else if(strcmp(cmd, "err")==0)
			{
				what=&c_err;which=-1;
			}
			else if(strcmp(cmd, "unk")==0)
			{
				what=&c_unk;which=-1;
			}
			else if(strcmp(cmd, "unn")==0)
			{
				what=&c_unn;which=-1;
			}
			if(what)
			{
				char *spec=strtok(NULL, "");
				colour new;int hi, ul;
				sscanf(spec, "%d %d %d %d", &new.fore, &new.back, &hi, &ul);
				new.hi=hi;new.ul=ul;
				if(which!=2)
					what[0]=new;
				if((which%2)==0)
					what[1]=new;
			}
			else
			{
				char msg[48+strlen(cmd)];
				sprintf(msg, "rc: Unrecognised ident in %%colour (%s)", cmd);
				atr_failsafe(&s_buf, ERR, msg, "init: ");
				nerrors++;
			}
		}
		else
		{
			char *rest=strtok(NULL, "");
			bool need=true;
#include "config_need.c"
			if(need&&!rest)
			{
				char msg[40+strlen(cmd)];
				sprintf(msg, "rc: Command (%s) without argument", cmd);
				atr_failsafe(&s_buf, ERR, msg, "init: ");
				nerrors++;
			}
			else if(strcmp(cmd, "server")==0)
			{
				servlist * new=(servlist *)malloc(sizeof(servlist));
				new->next=servs;
				new->name=strdup(rest);
				new->nick=strdup(nick);
				new->portno=strdup(portno);
				new->pass=pass?strdup(pass):NULL;
				new->join=false;
				new->chans=NULL;
				new->igns=NULL;
				servs=new;
			}
			else if(servs && (strcmp(cmd, "*port")==0))
			{
				free(servs->portno);
				servs->portno=strdup(rest);
			}
			else if(strcmp(cmd, "port")==0)
			{
				free(portno);
				portno=strdup(rest);
			}
			else if(strcmp(cmd, "pass")==0)
			{
				free(pass);
				pass=strdup(rest);
			}
			else if(servs && (strcmp(cmd, "*pass")==0))
			{
				free(servs->pass);
				servs->pass=strdup(rest);
			}
			else if(strcmp(cmd, "uname")==0)
			{
				username=strdup(rest);
				if(defnick)
				{
					free(nick);
					nick=strdup(username);
				}
			}
			else if(strcmp(cmd, "fname")==0)
			{
				free(fname);
				fname=strdup(rest);
			}
			else if(servs && (strcmp(cmd, "*nick")==0))
			{
				free(servs->nick);
				servs->nick=strdup(rest);
			}
			else if(strcmp(cmd, "nick")==0)
			{
				free(nick);
				nick=strdup(rest);
				defnick=false;
			}
			else if(strcmp(cmd, "ignore")==0)
			{
				char *sw=strtok(rest, " \t");
				if(*sw!='-')
				{
					atr_failsafe(&s_buf, ERR, "rc: ignore: need options (use '-' for no options)", "init: ");
					nerrors++;
				}
				else
				{
					rest=strtok(NULL, "");
					if(!rest)
					{
						atr_failsafe(&s_buf, ERR, "rc: ignore: need options (use '-' for no options)", "init: ");
						nerrors++;
					}
					else
					{
						bool icase=strchr(sw, 'i');
						bool pms=strchr(sw, 'p');
						bool regex=strchr(sw, 'r');
						if(regex)
						{
							name *new=n_add(&igns, rest, RFC1459);
							if(new)
							{
								new->icase=icase;
								new->pms=pms;
							}
						}
						else
						{
							char *iusr=strtok(rest, "@");
							char *ihst=strtok(NULL, "");
							if((!iusr) || (*iusr==0) || (*iusr=='*'))
								iusr="[^@]*";
							if((!ihst) || (*ihst==0) || (*ihst=='*'))
								ihst="[^@]*";
							char expr[10+strlen(iusr)+strlen(ihst)];
							sprintf(expr, "^%s[_~]*@%s$", iusr, ihst);
							name *new=n_add(&igns, expr, RFC1459);
							if(new)
							{
								new->icase=icase;
								new->pms=pms;
							}
						}
					}
				}
			}
			else if(servs && (strcmp(cmd, "*ignore")==0))
			{
				char *sw=strtok(rest, " \t");
				if(*sw!='-')
				{
					atr_failsafe(&s_buf, ERR, "rc: *ignore: need options (use '-' for no options)", "init: ");
					nerrors++;
				}
				else
				{
					rest=strtok(NULL, "");
					if(!rest)
					{
						atr_failsafe(&s_buf, ERR, "rc: *ignore: need options (use '-' for no options)", "init: ");
						nerrors++;
					}
					else
					{
						bool icase=strchr(sw, 'i');
						bool pms=strchr(sw, 'p');
						bool regex=strchr(sw, 'r');
						if(regex)
						{
							name *new=n_add(&servs->igns, rest, RFC1459);
							if(new)
							{
								new->icase=icase;
								new->pms=pms;
							}
						}
						else
						{
							char *isrc,*iusr,*ihst;
							prefix_split(rest, &isrc, &iusr, &ihst);
							if((!isrc) || (*isrc==0) || (*isrc=='*'))
								isrc="[^!@]*";
							if((!iusr) || (*iusr==0) || (*iusr=='*'))
								iusr="[^!@]*";
							if((!ihst) || (*ihst==0) || (*ihst=='*'))
								ihst="[^@]*";
							char expr[16+strlen(isrc)+strlen(iusr)+strlen(ihst)];
								sprintf(expr, "^%s[_~]*!%s@%s$", isrc, iusr, ihst);
							name *new=n_add(&servs->igns, expr, RFC1459);
							if(new)
							{
								new->icase=icase;
								new->pms=pms;
							}
						}
					}
				}
			}
			else if(servs && (strcmp(cmd, "*chan")==0))
			{
				chanlist * new=(chanlist *)malloc(sizeof(chanlist));
				new->next=servs->chans;
				new->name=strdup(rest);
				if((new->key=strpbrk(new->name, " \t")))
				{
					*new->key++=0;
					new->key=strdup(new->key);
				}
				new->igns=NULL;
				new->logt=LOGT_NONE;
				new->logf=NULL;
				servs->chans=new;
			}
			else if(servs && servs->chans && (strcmp(cmd, ">log")==0))
			{
				if(servs->chans->logf) fclose(servs->chans->logf);
				char *type=strtok(rest, " \t");
				logtype logt=LOGT_NONE;
				if(strcasecmp(type, "plain")==0)
					logt=LOGT_PLAIN;
				else if(strcasecmp(type, "symbolic")==0)
					logt=LOGT_SYMBOLIC;
				else
				{
					atr_failsafe(&s_buf, ERR, "rc: >log: Unrecognised log type (valid types are: plain, symbolic)", "init: ");
					nerrors++;
				}
				if(logt!=LOGT_NONE)
				{
					char *logf=strtok(NULL, "");
					servs->chans->logf=fopen(logf, "a");
					if(!servs->chans->logf)
					{
						atr_failsafe(&s_buf, ERR, "rc: >log: Failed to open log file for append", "init: ");
						atr_failsafe(&s_buf, ERR, strerror(errno), "fopen: ");
						nerrors++;
					}
					servs->chans->logt=logt;
				}
			}
#include "config_rcread.c"
			else
			{
				char msg[48+strlen(cmd)];
				sprintf(msg, "rc: Unrecognised cmd %s in .quirc/rc (ignoring)", cmd);
				atr_failsafe(&s_buf, ERR, msg, "init: ");
				nerrors++;
			}
		}
		free(line);
	}
	return(nerrors);
}

signed int pargs(int argc, char *argv[])
{
	bool check=false;
	int arg;
	for(arg=1;arg<argc;arg++)
	{
		if((strcmp(argv[arg], "--help")==0)||(strcmp(argv[arg], "-h")==0))
		{
			fprintf(stderr, USAGE_MSG);
			fprintf(stderr, "options:\n");
#include "config_help.c"
			return(0);
		}
		else if((strcmp(argv[arg], "--version")==0)||(strcmp(argv[arg], "-V")==0)||(strcmp(argv[arg], "-v")==0)) // allow -v as we did in old versions; depr
		{
			fprintf(stderr, VERSION_MSG);
			return(0);
		}
		else if((strcmp(argv[arg], "--no-server")==0)||(strcmp(argv[arg], "--no-auto-connect")==0)) // the "-auto" forms are from older versions; depr
		{
			freeservlist(servs);
			servs=NULL;
		}
		else if(servs && ((strcmp(argv[arg], "--no-chan")==0)||(strcmp(argv[arg], "--no-auto-join")==0)))
		{
			autojoin=false;
		}
		else if((strcmp(argv[arg], "--check")==0)||(strcmp(argv[arg], "--lint")==0))
		{
			check=true;
		}
		else if(strncmp(argv[arg], "--server=", 9)==0)
		{
			servs=(servlist *)malloc(sizeof(servlist));
			servs->next=NULL;
			servs->name=strdup(argv[arg]+9);
			servs->nick=strdup(nick);
			servs->portno=strdup(portno);
			servs->join=false;
			servs->chans=NULL;
			servs->igns=NULL;
		}
		else if(strncmp(argv[arg], "--port=", 7)==0)
		{
			free(portno);
			portno=strdup(argv[arg]+7);
		}
		else if(strncmp(argv[arg], "--pass=", 7)==0)
		{
			free(pass);
			pass=strdup(argv[arg]+7);
		}
		else if(strncmp(argv[arg], "--uname=", 8)==0)
		{
			free(username);
			username=strdup(argv[arg]+8);
			if(defnick)
			{
				free(nick);
				nick=strdup(username);
			}
		}
		else if(strncmp(argv[arg], "--fname=", 8)==0)
		{
			free(fname);
			fname=argv[arg]+8;
		}
		else if(strncmp(argv[arg], "--nick=", 7)==0)
		{
			free(nick);
			nick=strdup(argv[arg]+7);
			defnick=false;
		}
		else if(servs && (strncmp(argv[arg], "--chan=", 7)==0))
		{
			chanlist * new=(chanlist *)malloc(sizeof(chanlist));
			new->next=servs->chans;
			new->name=strdup(argv[arg]+7);
			if((new->key=strpbrk(new->name, " \t")))
			{
				*new->key++=0;
				new->key=strdup(new->key);
			}
			new->igns=NULL;
			new->logt=LOGT_NONE;
			new->logf=NULL;
			servs->chans=new;
		}
#include "config_pargs.c"
		else
		{
			char msg[40+strlen(argv[arg])];
			sprintf(msg, "pargs: Unrecognised argument '%s'", argv[arg]);
			atr_failsafe(&s_buf, ERR, msg, "init: ");
		}
	}
	if(check) return(0);
	return(-1);
}

void freeservlist(servlist *serv)
{
	if(serv)
	{
		free(serv->name);
		free(serv->nick);
		free(serv->portno);
		free(serv->pass);
		n_free(serv->igns);
		freechanlist(serv->chans);
		freeservlist(serv->next);
	}
	free(serv);
}

void freechanlist(chanlist *chan)
{
	if(chan)
	{
		free(chan->name);
		free(chan->key);
		if(chan->logf) fclose(chan->logf);
		n_free(chan->igns);
		freechanlist(chan->next);
	}
	free(chan);
}
