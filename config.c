/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010 Edward Cree

	See quirc.c for license information
	config: rc file and option parsing
*/

#include "config.h"

int def_config(void)
{
	buflines=256;
	mirc_colour_compat=1; // silently strip
	force_redraw=2;
	full_width_colour=false;
	hilite_tabstrip=false;
	tsb=true; // show top status bar
	autojoin=true;
	char *cols=getenv("COLUMNS"), *rows=getenv("LINES");
	if(cols) sscanf(cols, "%u", &width);
	if(rows) sscanf(rows, "%u", &height);
	if(!width) width=80;
	if(!height) height=24;
	if(width<30)
	{
		width=30;
		printf("width set to minimum 30\n");
	}
	if(height<5)
	{
		height=5;
		printf("height set to minimum 5\n");
	}
	maxnlen=16;
	servs=NULL;
	igns=NULL;
	portno="6667";
	username="quirc";
	fname=(char *)malloc(64+strlen(VERSION_TXT));
	nick=strdup("ac");
	sprintf(fname, "quIRC %hhu.%hhu.%hhu%s%s : http://github.com/ec429/quIRC", VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_TXT[0]?"-":"", VERSION_TXT);
	sprintf(version, "%hhu.%hhu.%hhu%s%s", VERSION_MAJ, VERSION_MIN, VERSION_REV, VERSION_TXT[0]?"-":"", VERSION_TXT);
	return(0);
}

int rcread(FILE *rcfp)
{
	while(!feof(rcfp))
	{
		char *line=fgetl(rcfp);
		char *cmd=strtok(line, " \t");
		if((*cmd=='#') || (*cmd=='\n') || (!*cmd)) // #lines are comments, blank lines are ignored
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
				char *spec=strtok(NULL, "\n");
				colour new;int hi, ul;
				sscanf(spec, "%d %d %d %d", &new.fore, &new.back, &hi, &ul);
				new.hi=hi;new.ul=ul;
				if(which!=2)
					what[0]=new;
				if((which%2)==0)
					what[1]=new;
			}
		}
		else
		{
			char *rest=strtok(NULL, "\n");
			if(strcmp(cmd, "server")==0)
			{
				servlist * new=(servlist *)malloc(sizeof(servlist));
				new->next=servs;
				new->name=strdup(rest);
				new->nick=strdup(nick);
				new->portno=strdup(portno);
				new->join=false;
				new->chans=NULL;
				new->igns=NULL;
				servs=new;
			}
			else if(servs && (strcmp(cmd, "*port")==0))
			{
				if(servs->portno) free(servs->portno);
				servs->portno=strdup(rest);
			}
			else if(strcmp(cmd, "port")==0)
				portno=strdup(rest);
			else if(strcmp(cmd, "uname")==0)
				username=strdup(rest);
			else if(strcmp(cmd, "fname")==0)
				fname=strdup(rest);
			else if(servs && (strcmp(cmd, "*nick")==0))
			{
				if(servs->nick) free(servs->nick);
				servs->nick=strdup(rest);
			}
			else if(strcmp(cmd, "nick")==0)
				nick=strdup(rest);
			else if(strcmp(cmd, "ignore")==0)
			{
				char *sw=strtok(rest, " \t");
				rest=strtok(NULL, "");
				bool icase=strchr(sw, 'i');
				bool pms=strchr(sw, 'p');
				bool regex=strchr(sw, 'r');
				if(regex)
				{
					name *new=n_add(&igns, rest);
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
					name *new=n_add(&igns, expr);
					if(new)
					{
						new->icase=icase;
						new->pms=pms;
					}
				}
			}
			else if(servs && (strcmp(cmd, "*ignore")==0))
			{
				char *sw=strtok(rest, " \t");
				rest=strtok(NULL, "");
				bool icase=strchr(sw, 'i');
				bool pms=strchr(sw, 'p');
				bool regex=strchr(sw, 'r');
				if(regex)
				{
					name *new=n_add(&servs->igns, rest);
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
					name *new=n_add(&servs->igns, expr);
					if(new)
					{
						new->icase=icase;
						new->pms=pms;
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
				}
				new->igns=NULL;
				servs->chans=new;
			}
			else if(strcmp(cmd, "mnln")==0)
				sscanf(rest, "%u", &maxnlen);
			else if(strcmp(cmd, "mcc")==0)
				sscanf(rest, "%u", &mirc_colour_compat);
			else if(strcmp(cmd, "fwc")==0)
			{
				int fwc;
				sscanf(rest, "%u", &fwc);
				full_width_colour=fwc;
			}
			else if(strcmp(cmd, "hts")==0)
			{
				int hts;
				sscanf(rest, "%u", &hts);
				hilite_tabstrip=hts;
			}
			else if(strcmp(cmd, "tsb")==0)
			{
				int ntsb;
				sscanf(rest, "%u", &ntsb);
				tsb=ntsb;
			}
			else if(strcmp(cmd, "fred")==0)
				sscanf(rest, "%u", &force_redraw);
			else if(strcmp(cmd, "buf")==0)
				sscanf(rest, "%u", &buflines);
			else
			{
				fprintf(stderr, "Unrecognised cmd %s in .quirc (ignoring)\n", cmd);
			}
		}
		free(line);
	}
	return(0);
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
			return(0);
		}
		else if((strcmp(argv[arg], "--version")==0)||(strcmp(argv[arg], "-V")==0)||(strcmp(argv[arg], "-v")==0)) // allow -v as we did in old versions; depr
		{
			fprintf(stderr, VERSION_MSG);
			return(0);
		}
		else if(strncmp(argv[arg], "--width=", 8)==0) // just in case you need to force them
		{
			sscanf(argv[arg]+8, "%u", &width);
			if(width<30)
			{
				width=30;
				printf("width set to minimum 30\n");
			}
		}
		else if(strncmp(argv[arg], "--height=", 9)==0)
		{
			sscanf(argv[arg]+9, "%u", &height);
			if(height<5)
			{
				height=5;
				printf("height set to minimum 5\n");
			}
		}
		else if(strncmp(argv[arg], "--maxnicklen=", 13)==0)
		{
			sscanf(argv[arg]+13, "%u", &maxnlen);
		}
		else if(strncmp(argv[arg], "--mcc=", 6)==0)
		{
			sscanf(argv[arg]+6, "%u", &mirc_colour_compat);
		}
		else if(strcmp(argv[arg], "--fwc")==0)
		{
			full_width_colour=true;
		}
		else if(strcmp(argv[arg], "--no-fwc")==0)
		{
			full_width_colour=false;
		}
		else if(strcmp(argv[arg], "--hts")==0)
		{
			hilite_tabstrip=true;
		}
		else if(strcmp(argv[arg], "--no-hts")==0)
		{
			hilite_tabstrip=false;
		}
		else if(strcmp(argv[arg], "--tsb")==0)
		{
			tsb=true;
		}
		else if(strcmp(argv[arg], "--no-tsb")==0)
		{
			tsb=false;
		}
		else if(strncmp(argv[arg], "--force-redraw=", 15)==0)
		{
			sscanf(argv[arg]+15, "%u", &force_redraw);
		}
		else if(strncmp(argv[arg], "--buf-lines=", 12)==0)
		{
			sscanf(argv[arg]+12, "%u", &buflines);
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
			portno=argv[arg]+7;
		else if(strncmp(argv[arg], "--uname=", 8)==0)
			username=argv[arg]+8;
		else if(strncmp(argv[arg], "--fname=", 8)==0)
			fname=argv[arg]+8;
		else if(strncmp(argv[arg], "--nick=", 7)==0)
			nick=strdup(argv[arg]+7);
		else if(strncmp(argv[arg], "--chan=", 7)==0)
		{
			chanlist * new=(chanlist *)malloc(sizeof(chanlist));
			new->next=servs->chans;
			new->name=strdup(argv[arg]+7);
			if((new->key=strpbrk(new->name, " \t")))
			{
				*new->key++=0;
			}
			new->igns=NULL;
			servs->chans=new;
		}
		else
		{
			fprintf(stderr, "Unrecognised argument '%s'\n", argv[arg]);
		}
	}
	if(check) return(0);
	return(-1);
}

void freeservlist(servlist * serv)
{
	if(serv)
	{
		if(serv->name) free(serv->name);
		if(serv->nick) free(serv->nick);
		if(serv->portno) free(serv->portno);
		freechanlist(serv->chans);
		freeservlist(serv->next);
	}
}

void freechanlist(chanlist * chan)
{
	if(chan)
	{
		if(chan->name) free(chan->name);
		if(chan->key) free(chan->key);
		freechanlist(chan->next);
	}
}
