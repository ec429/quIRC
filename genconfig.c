/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	genconfig.c: generate various bits of config-parsing code from a cdl file
*/

// TODO: refactor this quite a lot, it's a mess

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "strbuf.h"

typedef struct
{
	enum {INT, BOOL} type;
	char *cname;
	int value;
	int min,max;
	char *rc_name;
	char *cmdline_name;
	char *set_name;
	char *set_msg;
	enum {SET, LEVEL, BOOLEAN} set_type;
}
ent;

int main(int argc, char **argv)
{
	if(argc!=2)
	{
		fprintf(stderr, "usage: genconfig <outtype>\n");
		return(EXIT_FAILURE);
	}
	int otype;
	if(strcmp(argv[1], "config_globals.h")==0)
		otype=0;
	else if(strcmp(argv[1], "config_check.c")==0)
		otype=1;
	else if(strcmp(argv[1], "config_def.c")==0)
		otype=2;
	else if(strcmp(argv[1], "config_rcread.c")==0)
		otype=3;
	else if(strcmp(argv[1], "config_pargs.c")==0)
		otype=4;
	else if(strcmp(argv[1], "config_set.c")==0)
		otype=5;
	else if(strcmp(argv[1], "config_need.c")==0)
		otype=6;
	else if(strcmp(argv[1], "config_help.c")==0)
		otype=7;
	else if(strcmp(argv[1], "config_ref.htm")==0)
		otype=8;
	else
	{
		fprintf(stderr, "genconfig: unrecognised outtype '%s'\n", argv[1]);
		return(EXIT_FAILURE);
	}
	int nents=0;
	ent *ents=NULL;
	while(!feof(stdin))
	{
		char *line=fgetl(stdin);
		if(!*line)
		{
			free(line);
			continue;
		}
		int nent=nents++;
		ents=realloc(ents, nents*sizeof(ent));
		if(!ents)
		{
			perror("genconfig: realloc");
			return(EXIT_FAILURE);
		}
		char *p=line;
		int tocolon=strcspn(p, ":");
		if(strncmp(p, "int", tocolon)==0)
			ents[nent].type=INT;
		else if(strncmp(p, "bool", tocolon)==0)
			ents[nent].type=BOOL;
		else
		{
			fprintf(stderr, "genconfig: unrecognised type %.*s\n", tocolon, p);
			return(EXIT_FAILURE);
		}
		p+=tocolon;
		if(!*p)
		{
			fprintf(stderr, "genconfig: premature end of line %u\n", nents);
			return(EXIT_FAILURE);
		}
		p++;tocolon=strcspn(p, ":");
		ents[nent].cname=malloc(tocolon+1);
		if(!ents[nent].cname)
		{
			perror("genconfig: malloc");
			return(EXIT_FAILURE);
		}
		strncpy(ents[nent].cname, p, tocolon);
		ents[nent].cname[tocolon]=0;
		p+=tocolon;
		if(!*p)
		{
			fprintf(stderr, "genconfig: premature end of line %u (%s)\n", nents, ents[nent].cname);
			return(EXIT_FAILURE);
		}
		p++;tocolon=strcspn(p, ":");
		if(tocolon)
		{
			char *value=malloc(tocolon+1);
			if(!value)
			{
				perror("genconfig: malloc");
				return(EXIT_FAILURE);
			}
			strncpy(value, p, tocolon);
			value[tocolon]=0;
			switch(ents[nent].type)
			{
				case INT:
					if(!sscanf(value, "%d", &ents[nent].value)) ents[nent].value=-1;
				break;
				case BOOL:
					if(strcmp(value, "true")==0)
						ents[nent].value=1;
					else if(strcmp(value, "false")==0)
						ents[nent].value=0;
					else if(!sscanf(value, "%d", &ents[nent].value))
						ents[nent].value=1; // default to true
				break;
			}
			free(value);
			p+=tocolon;
		}
		else
		{
			ents[nent].value=-1;
		}
		if(!*p)
		{
			fprintf(stderr, "genconfig: premature end of line %u (%s)\n", nents, ents[nent].cname);
			return(EXIT_FAILURE);
		}
		p++;tocolon=strcspn(p, ":");
		if(tocolon)
		{
			char *min=malloc(tocolon+1);
			if(!min)
			{
				perror("genconfig: malloc");
				return(EXIT_FAILURE);
			}
			strncpy(min, p, tocolon);
			min[tocolon]=0;
			if(!sscanf(min, "%d", &ents[nent].min)) ents[nent].min=-1;
			free(min);
			p+=tocolon;
		}
		else
		{
			ents[nent].min=-1;
		}
		if(!*p)
		{
			fprintf(stderr, "genconfig: premature end of line %u (%s)\n", nents, ents[nent].cname);
			return(EXIT_FAILURE);
		}
		p++;tocolon=strcspn(p, ":");
		if(tocolon)
		{
			char *max=malloc(tocolon+1);
			if(!max)
			{
				perror("genconfig: malloc");
				return(EXIT_FAILURE);
			}
			strncpy(max, p, tocolon);
			max[tocolon]=0;
			if(!sscanf(max, "%d", &ents[nent].max)) ents[nent].max=-1;
			free(max);
			p+=tocolon;
		}
		else
		{
			ents[nent].max=-1;
		}
		if(!*p)
		{
			fprintf(stderr, "genconfig: premature end of line %u (%s)\n", nents, ents[nent].cname);
			return(EXIT_FAILURE);
		}
		p++;tocolon=strcspn(p, ":");
		if(tocolon)
		{
			ents[nent].rc_name=malloc(tocolon+1);
			if(!ents[nent].rc_name)
			{
				perror("genconfig: malloc");
				return(EXIT_FAILURE);
			}
			strncpy(ents[nent].rc_name, p, tocolon);
			ents[nent].rc_name[tocolon]=0;
			p+=tocolon;
		}
		else
		{
			ents[nent].rc_name=NULL;
		}
		if(!*p)
		{
			fprintf(stderr, "genconfig: premature end of line %u (%s)\n", nents, ents[nent].cname);
			return(EXIT_FAILURE);
		}
		p++;tocolon=strcspn(p, ":");
		if(tocolon)
		{
			ents[nent].cmdline_name=malloc(tocolon+1);
			if(!ents[nent].cmdline_name)
			{
				perror("genconfig: malloc");
				return(EXIT_FAILURE);
			}
			strncpy(ents[nent].cmdline_name, p, tocolon);
			ents[nent].cmdline_name[tocolon]=0;
			p+=tocolon;
		}
		else
		{
			ents[nent].cmdline_name=NULL;
		}
		if(!*p)
		{
			fprintf(stderr, "genconfig: premature end of line %u (%s)\n", nents, ents[nent].cname);
			return(EXIT_FAILURE);
		}
		p++;tocolon=strcspn(p, ":");
		if(tocolon)
		{
			ents[nent].set_name=malloc(tocolon+1);
			if(!ents[nent].set_name)
			{
				perror("genconfig: malloc");
				return(EXIT_FAILURE);
			}
			strncpy(ents[nent].set_name, p, tocolon);
			ents[nent].set_name[tocolon]=0;
			p+=tocolon;
		}
		else
		{
			ents[nent].set_name=NULL;
		}
		if(!*p)
		{
			fprintf(stderr, "genconfig: premature end of line %u (%s)\n", nents, ents[nent].cname);
			return(EXIT_FAILURE);
		}
		p++;tocolon=strcspn(p, ":");
		if(strncmp(p, "SET", tocolon)==0)
			ents[nent].set_type=SET;
		else if(strncmp(p, "LEVEL", tocolon)==0)
			ents[nent].set_type=LEVEL;
		else if(strncmp(p, "BOOLEAN", tocolon)==0)
			ents[nent].set_type=BOOLEAN;
		else
		{
			fprintf(stderr, "genconfig: unrecognised set_type %.*s in line %u (%s)\n", tocolon, p, nents, ents[nent].cname);
			return(EXIT_FAILURE);
		}
		p+=tocolon;
		if(!*p)
		{
			fprintf(stderr, "genconfig: premature end of line %u (%s)\n", nents, ents[nent].cname);
			return(EXIT_FAILURE);
		}
		p++;tocolon=strcspn(p, ":");
		if(tocolon)
		{
			ents[nent].set_msg=malloc(tocolon+1);
			if(!ents[nent].set_msg)
			{
				perror("genconfig: malloc");
				return(EXIT_FAILURE);
			}
			strncpy(ents[nent].set_msg, p, tocolon);
			ents[nent].set_msg[tocolon]=0;
		}
		else
		{
			ents[nent].set_msg=NULL;
		}
		free(line);
	}
	if(otype==8)
		printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\">\n\
<head>\n\
\t<title>quIRC: configuration reference</title>\n\
\t<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8;\" />\n\
\t<link rel=\"stylesheet\" href=\"readme.css\" />\n\t<style type=\"text/css\">\n\
\t\ttt {margin-left:4em;}\n\
\t\ttd {border:1px solid; border-color:#1f273f; padding:2px;}\n\
\t\t.int {background-color:#376f4f; color:black;}\n\
\t\t.bool {background-color:#374f6f; color:black;}\n\
\t\t.na {background-color:#2f2f37; color:#7f7f9f};\n\
\t</style>\n\
</head>\n\
<body>\n\
<div id=\"title\">\n\
<h1>quIRC: reference</h1>\n\
<h5>A reference table for quIRC's configuration options, and how to set them.<br>\n\
Generated by <small>genconfig</small></h5>\n\
</div><!--#title-->\n\
<div id=\"key\">\n\
<h2>Key to Columns</h2>\n\
<dl>\n\
<dt>name</dt>\n\
\t<dd>The internal name of the setting.</dd>\n\
<dt>type</dt>\n\
\t<dd>The setting's type, either <span class=\"int\">INT</span> (integer) or <span class=\"bool\">BOOL</span> (boolean).</dd>\n\
<dt>default</dt>\n\
\t<dd>The setting's default value.</dd>\n\
<dt>rc</dt>\n\
\t<dd>The setting's name in the rc file (typically ~/.quirc/rc), if settable there.</dd>\n\
<dt>cmdline</dt>\n\
\t<dd>The setting's name on the command line, if settable there.</dd>\n\
<dt>/set</dt>\n\
\t<dd>The name to use with the /set command.  If the setting is an <span class=\"int\">INT</span>, use <small>/set &lt;name&gt; &lt;value&gt;</small>; omitting <small>&lt;value&gt;</small> sets to the default value.  If the setting is a <span class=\"bool\">BOOL</span>, use <small>/set &lt;name&gt;</small> to enable, and <small>/set no-&lt;name&gt;</small> to disable (you can also use other forms, such as supplying a <small>&lt;value&gt;</small> of + or 1 to enable and - or 0 to disable).</dd>\n\
<dt>description</dt>\n\
\t<dd>A short description of the setting, as used in messages produced by /set.</dd>\n\
</dl>\n\
</div><!--#key-->\n\
<div id=\"table\">\n\
<table style=\"border:1px solid;\">\n\
<tr style=\"text-align:center\"><td>name</td><td>type</td><td>default</td><td>rc</td><td>cmdline</td><td>/set</td><td>description</td></tr>\n");
	else
		printf("/* Generated by genconfig */\n");
	switch(otype)
	{
		case 1:
			printf("int conf_check(void)\n{\n");
		break;
	}
	int i;
	bool first=true;
	for(i=0;i<nents;i++)
	{
		switch(otype)
		{
			case 0:
				switch(ents[i].type)
				{
					case INT:
						printf("unsigned int ");
					break;
					case BOOL:
						printf("bool ");
					break;
					default:
						fprintf(stderr, "Unsupported type %d in %s\n", ents[i].type, ents[i].cname);
						return(EXIT_FAILURE);
				}
				printf("%s; // %s\n", ents[i].cname, ents[i].set_msg);
			break;
			case 1:
				switch(ents[i].type)
				{
					case INT:
						if(ents[i].min>0)
						{
							printf("\tif(%s<%d)\n\t{\n\t\tatr_failsafe(&s_buf, MT_STATUS, \"%s set to minimum %d\", \"init: \");\n\t\t%s=%d;\n\t}\n", ents[i].cname, ents[i].min, ents[i].set_name, ents[i].min, ents[i].cname, ents[i].min);
						}
						if(ents[i].max!=-1)
						{
							printf("\tif(%s>%d)\n\t{\n\t\tatr_failsafe(&s_buf, MT_STATUS, \"%s set to maximum %d\", \"init: \");\n\t\t%s=%d;\n\t}\n", ents[i].cname, ents[i].max, ents[i].set_name, ents[i].max, ents[i].cname, ents[i].max);
						}
					break;
					case BOOL:
					break;
					default:
						fprintf(stderr, "Unsupported type %d in %s\n", ents[i].type, ents[i].cname);
						return(EXIT_FAILURE);
				}
			break;
			case 2:
				printf("\t%s=%d;\n",  ents[i].cname,  ents[i].value);
			break;
			case 3:
				if(ents[i].rc_name)
				{
					switch(ents[i].type)
					{
						case BOOL:
						case INT:
							if((ents[i].set_type==BOOLEAN)||(ents[i].set_type==LEVEL))
							{
								printf("\t\t\telse if(strcmp(cmd, \"no-%s\")==0)\n", ents[i].rc_name);
								printf("\t\t\t\t%s=false;\n", ents[i].cname);
							}
							printf("\t\t\telse if(strcmp(cmd, \"%s\")==0)\n", ents[i].rc_name);
							printf("\t\t\t{\n");
							printf("\t\t\t\tunsigned int value;\n");
							printf("\t\t\t\tif(rest&&sscanf(rest, \"%%u\", &value))\n");
							printf("\t\t\t\t\t%s=value;\n", ents[i].cname);
							printf("\t\t\t\telse\n");
							if(ents[i].type==BOOL)
								printf("\t\t\t\t\t%s=true;\n", ents[i].cname);
							else
							{
								printf("\t\t\t\t{\n");
								printf("\t\t\t\t\tatr_failsafe(&s_buf, MT_ERR, \"Malformed rc entry for %s (value not numeric)\", \"init: \");\n", ents[i].rc_name);
								printf("\t\t\t\t\tatr_failsafe(&s_buf, MT_ERR, rest, \"init: \");\n");
								printf("\t\t\t\t}\n");
							}
							printf("\t\t\t}\n");
						break;
						default:
							fprintf(stderr, "Unsupported type %d in %s\n", ents[i].type, ents[i].cname);
							return(EXIT_FAILURE);
					}
				}
			break;
			case 4:
				if(ents[i].cmdline_name)
				{
					switch(ents[i].type)
					{
						case BOOL:
							printf("\t\telse if(strcmp(argv[arg], \"--%s\")==0)\n", ents[i].cmdline_name);
							printf("\t\t\t%s=true;\n", ents[i].cname);
							printf("\t\telse if(strcmp(argv[arg], \"--no-%s\")==0)\n", ents[i].cmdline_name);
							printf("\t\t\t%s=false;\n", ents[i].cname);
						break;
						case INT:
							printf("\t\telse if(strncmp(argv[arg], \"--%s=\", %zu)==0)\n", ents[i].cmdline_name, strlen(ents[i].cmdline_name)+3);
							printf("\t\t\tsscanf(argv[arg]+%zu, \"%%u\", &%s);\n", strlen(ents[i].cmdline_name)+3, ents[i].cname);
						break;
					}
				}
			break;
			case 5:
				if(ents[i].set_name)
				{
					printf("\t\t\t\t");
					if(!first) printf("else ");
					first=false;
					printf("if(strcmp(opt, \"%s\")==0)\n", ents[i].set_name);
					printf("\t\t\t\t{\n");
					if(ents[i].set_type==BOOLEAN)
					{
						printf("\t\t\t\t\tif(val)\n");
						printf("\t\t\t\t\t{\n");
						printf("\t\t\t\t\t\tif(isdigit(*val))\n");
						printf("\t\t\t\t\t\t{\n");
						printf("\t\t\t\t\t\t\tunsigned int value;\n");
						printf("\t\t\t\t\t\t\tsscanf(val, \"%%u\", &value);\n");
						printf("\t\t\t\t\t\t\t%s=value;\n", ents[i].cname);
						printf("\t\t\t\t\t\t}\n");
						printf("\t\t\t\t\t\telse if(strcmp(val, \"+\")==0)\n");
						printf("\t\t\t\t\t\t{\n");
						printf("\t\t\t\t\t\t\t%s=true;\n", ents[i].cname);
						printf("\t\t\t\t\t\t}\n");
						printf("\t\t\t\t\t\telse if(strcmp(val, \"-\")==0)\n");
						printf("\t\t\t\t\t\t{\n");
						printf("\t\t\t\t\t\t\t%s=false;\n", ents[i].cname);
						printf("\t\t\t\t\t\t}\n");
						printf("\t\t\t\t\t\telse\n");
						printf("\t\t\t\t\t\t{\n");
						printf("\t\t\t\t\t\t\tadd_to_buffer(cbuf, MT_ERR, PRIO_NORMAL, 0, false, \"option '%s' is boolean, use only 0/1 or -/+ to set\", \"/set: \");\n", ents[i].set_name);
						printf("\t\t\t\t\t\t}\n");
						printf("\t\t\t\t\t}\n");
						printf("\t\t\t\t\telse\n");
						printf("\t\t\t\t\t\t%s=true;\n", ents[i].cname);
					}
					else
					{
						printf("\t\t\t\t\tif(val)\n");
						printf("\t\t\t\t\t{\n");
						printf("\t\t\t\t\t\tunsigned int value;\n");
						printf("\t\t\t\t\t\tsscanf(val, \"%%u\", &value);\n");
						printf("\t\t\t\t\t\t%s=value;\n", ents[i].cname);
						printf("\t\t\t\t\t}\n");
						printf("\t\t\t\t\telse\n");
						printf("\t\t\t\t\t\t%s=%d;\n", ents[i].cname, ents[i].value);
						if(ents[i].min>0)
						{
							printf("\t\t\t\t\tif(%s<%d)\n", ents[i].cname, ents[i].min);
							printf("\t\t\t\t\t\t%s=%d;\n", ents[i].cname, ents[i].min);
						}
						if(ents[i].max>=0)
						{
							printf("\t\t\t\t\tif(%s>%d)\n", ents[i].cname, ents[i].max);
							printf("\t\t\t\t\t\t%s=%d;\n", ents[i].cname, ents[i].max);
						}
					}
					switch(ents[i].set_type)
					{
						case BOOLEAN:
							printf("\t\t\t\t\tif(%s)\n", ents[i].cname);
							printf("\t\t\t\t\t\tadd_to_buffer(cbuf, MT_STATUS, PRIO_QUIET, 0, false, \"%s enabled\", \"/set: \");\n", ents[i].set_msg);
							printf("\t\t\t\t\telse\n");
							printf("\t\t\t\t\t\tadd_to_buffer(cbuf, MT_STATUS, PRIO_QUIET, 0, false, \"%s disabled\", \"/set: \");\n", ents[i].set_msg);
						break;
						case LEVEL:
							printf("\t\t\t\t\tif(%s)\n", ents[i].cname);
							printf("\t\t\t\t\t{\n");
							printf("\t\t\t\t\t\tchar lmsg[%zu];\n", strlen(ents[i].set_msg)+32);
							printf("\t\t\t\t\t\tsprintf(lmsg, \"%s level %%u enabled\", %s);\n", ents[i].set_msg, ents[i].cname);
							printf("\t\t\t\t\t\tadd_to_buffer(cbuf, MT_STATUS, PRIO_QUIET, 0, false, lmsg, \"/set: \");\n");
							printf("\t\t\t\t\t}\n");
							printf("\t\t\t\t\telse\n");
							printf("\t\t\t\t\t\tadd_to_buffer(cbuf, MT_STATUS, PRIO_QUIET, 0, false, \"%s disabled\", \"/set: \");\n", ents[i].set_msg);
						break;
						case SET:
							printf("\t\t\t\t\tchar smsg[%zu];\n", strlen(ents[i].set_msg)+24);
							printf("\t\t\t\t\tsprintf(smsg, \"%s set to %%u\", %s);\n", ents[i].set_msg, ents[i].cname);
							printf("\t\t\t\t\tadd_to_buffer(cbuf, MT_STATUS, PRIO_QUIET, 0, false, smsg, \"/set: \");\n");
						break;
					}
					printf("\t\t\t\t\tint buf;\n");
					printf("\t\t\t\t\tfor(buf=0;buf<nbufs;buf++)\n");
					printf("\t\t\t\t\t\tbufs[buf].dirty=true;\n");
					printf("\t\t\t\t\tredraw_buffer();\n");
					printf("\t\t\t\t}\n");
					if((ents[i].set_type==BOOLEAN)||(ents[i].set_type==LEVEL))
					{
						printf("\t\t\t\telse if(strcmp(opt, \"no-%s\")==0)\n", ents[i].set_name);
						printf("\t\t\t\t{\n");
						printf("\t\t\t\t\t%s=0;\n", ents[i].cname);
						printf("\t\t\t\t\tadd_to_buffer(cbuf, MT_STATUS, PRIO_QUIET, 0, false, \"%s disabled\", \"/set: \");\n", ents[i].set_msg);
						printf("\t\t\t\t\tint buf;\n");
						printf("\t\t\t\t\tfor(buf=0;buf<nbufs;buf++)\n");
						printf("\t\t\t\t\t\tbufs[buf].dirty=true;\n");
						printf("\t\t\t\t\tredraw_buffer();\n");
						printf("\t\t\t\t}\n");
					}
				}
			break;
			case 6:
				if(ents[i].rc_name)
				{
					printf("\t\t\tif(strcmp(cmd, \"%s\")==0) need=false;\n", ents[i].rc_name);
					if((ents[i].set_type==BOOLEAN)||(ents[i].set_type==LEVEL))
						printf("\t\t\tif(strcmp(cmd, \"no-%s\")==0) need=false;\n", ents[i].rc_name);
				}
			break;
			case 7:
				if(ents[i].cmdline_name)
				{
					int j;
					switch(ents[i].type)
					{
						case BOOL:
							printf("\t\t\tfprintf(stderr, \"\\t--[no-]%s", ents[i].cmdline_name);
							for(j=strlen(ents[i].cmdline_name);j<25;j++)
								putchar(' ');
						break;
						case INT:
							if((ents[i].min!=-1)&&(ents[i].max!=-1))
							{
								printf("\t\t\tfprintf(stderr, \"\\t--%s=<%d to %d>", ents[i].cmdline_name, ents[i].min, ents[i].max);
								for(j=strlen(ents[i].cmdline_name);j<21;j++)
									putchar(' ');
							}
							else
							{
								printf("\t\t\tfprintf(stderr, \"\\t--%s=<numeric>", ents[i].cmdline_name);
								for(j=strlen(ents[i].cmdline_name);j<20;j++)
									putchar(' ');
							}
						break;
					}
					printf(": %s\\n\");\n", ents[i].set_msg);
				}
			break;
			case 8:
				switch(ents[i].type)
				{
					case BOOL:
						printf("<tr><td>%s</td><td><span class=\"bool\">BOOL</span></td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n", ents[i].cname, ents[i].value?"true":"false", ents[i].rc_name?ents[i].rc_name:"<span class=\"na\">[N/A]</span>", ents[i].cmdline_name?ents[i].cmdline_name:"<span class=\"na\">[N/A]</span>", ents[i].set_name?ents[i].set_name:"<span class=\"na\">[N/A]</span>", ents[i].set_msg?ents[i].set_msg:"<span class=\"na\">[N/A]</span>");
					break;
					case INT:
						printf("<tr><td>%s</td><td><span class=\"int\">INT</span></td><td>%d</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n", ents[i].cname, ents[i].value, ents[i].rc_name?ents[i].rc_name:"<span class=\"na\">[N/A]</span>", ents[i].cmdline_name?ents[i].cmdline_name:"<span class=\"na\">[N/A]</span>", ents[i].set_name?ents[i].set_name:"<span class=\"na\">[N/A]</span>", ents[i].set_msg?ents[i].set_msg:"<span class=\"na\">[N/A]</span>");
					break;
					default:
						printf("<tr><td>%s</td><td><em>(unknown type)</em></td><td>%d</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n", ents[i].cname, ents[i].value, ents[i].rc_name?ents[i].rc_name:"<span class=\"na\">[N/A]</span>", ents[i].cmdline_name?ents[i].cmdline_name:"<span class=\"na\">[N/A]</span>", ents[i].set_name?ents[i].set_name:"<span class=\"na\">[N/A]</span>", ents[i].set_msg?ents[i].set_msg:"<span class=\"na\">[N/A]</span>");
					break;
				}
			break;
			default:
				fprintf(stderr, "genconfig: otype %d not implemented!\n", otype);
				return(EXIT_FAILURE);
			break;
		}
	}
	switch(otype)
	{
		case 1:
			printf("\treturn(0);\n}\n");
		break;
		case 8:
			printf("</table>\n</div><!--#table-->\n</body>\n</html>\n");
		break;
	}
	return(0);
}
