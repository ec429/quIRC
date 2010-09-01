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
	server=NULL;
	portno="6667";
	username="quirc";
	fname=(char *)malloc(64+strlen(VERSION_TXT));
	nick=strdup("ac");
	chan=NULL;
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
				server=strdup(rest);
			else if(strcmp(cmd, "port")==0)
				portno=strdup(rest);
			else if(strcmp(cmd, "uname")==0)
				username=strdup(rest);
			else if(strcmp(cmd, "fname")==0)
				fname=strdup(rest);
			else if(strcmp(cmd, "nick")==0)
				nick=strdup(rest);
			else if(strcmp(cmd, "chan")==0)
				chan=strdup(rest);
			else if(strcmp(cmd, "mnln")==0)
				sscanf(rest, "%u", &maxnlen);
			else if(strcmp(cmd, "mcc")==0)
				sscanf(rest, "%u", &mirc_colour_compat);
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
			server=NULL;
		}
		else if((strcmp(argv[arg], "--no-chan")==0)||(strcmp(argv[arg], "--no-auto-join")==0))
		{
			chan=NULL;
		}
		else if((strcmp(argv[arg], "--check")==0)||(strcmp(argv[arg], "--lint")==0))
		{
			check=true;
		}
		else if(strncmp(argv[arg], "--server=", 9)==0)
			server=argv[arg]+9;
		else if(strncmp(argv[arg], "--port=", 7)==0)
			portno=argv[arg]+7;
		else if(strncmp(argv[arg], "--uname=", 8)==0)
			username=argv[arg]+8;
		else if(strncmp(argv[arg], "--fname=", 8)==0)
			fname=argv[arg]+8;
		else if(strncmp(argv[arg], "--nick=", 7)==0)
			nick=strdup(argv[arg]+7);
		else if(strncmp(argv[arg], "--chan=", 7)==0)
			chan=argv[arg]+7;
		else
		{
			fprintf(stderr, "Unrecognised argument '%s'\n", argv[arg]);
		}
	}
	if(check) return(0);
	return(-1);
}
