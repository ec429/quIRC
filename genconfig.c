/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-11 Edward Cree

	See quirc.c for license information
	genconfig.c: generate various bits of config-parsing code from a cdl file
*/

// TODO: refactor this quite a lot, it's a mess

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

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

char * fgetl(FILE *); // gets a line of string data; returns a malloc-like pointer
void init_char(char **buf, int *l, int *i); // initialises a string buffer in heap.  *buf becomes a malloc-like pointer
void append_char(char **buf, int *l, int *i, char c); // adds a character to a string buffer in heap (and realloc()s if needed)

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
			if(!sscanf(value, "%d", &ents[nent].value)) ents[nent].value=-1;
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
							printf("\tif(%s<%d)\n\t{\n\t\tasb_failsafe(c_status, \"%s set to minimum %d\");\n\t\t%s=%d;\n\t}\n", ents[i].cname, ents[i].min, ents[i].set_name, ents[i].min, ents[i].cname, ents[i].min);
						}
						if(ents[i].max!=-1)
						{
							printf("\tif(%s>%d)\n\t{\n\t\tasb_failsafe(c_status, \"%s set to maximum %d\");\n\t\t%s=%d;\n\t}\n", ents[i].cname, ents[i].max, ents[i].set_name, ents[i].max, ents[i].cname, ents[i].max);
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
								printf("\t\t\t\t\tasb_failsafe(c_err, \"Malformed rc entry for %s (value not numeric)\");\n", ents[i].rc_name);
								printf("\t\t\t\t\tasb_failsafe(c_err, rest);\n");
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
							printf("\t\telse if(strncmp(argv[arg], \"--%s=\", %u)==0)\n", ents[i].cmdline_name, strlen(ents[i].cmdline_name)+3);
							printf("\t\t\tsscanf(argv[arg]+%u, \"%%u\", &%s);\n", strlen(ents[i].cmdline_name)+3, ents[i].cname);
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
					printf(        "if(strcmp(opt, \"%s\")==0)\n", ents[i].set_name);
					printf("\t\t\t\t{\n");
					printf("\t\t\t\t\tif(val)\n");
					printf("\t\t\t\t\t{\n");
					printf("\t\t\t\t\t\tunsigned int value;\n");
					printf("\t\t\t\t\t\tsscanf(val, \"%%u\", &value);\n");
					printf("\t\t\t\t\t\t%s=value;\n", ents[i].cname);
					printf("\t\t\t\t\t}\n");
					printf("\t\t\t\t\telse\n");
					printf("\t\t\t\t\t\t%s=%d;\n", ents[i].cname, ents[i].value);
					switch(ents[i].set_type)
					{
						case BOOLEAN:
							printf("\t\t\t\t\tif(%s)\n", ents[i].cname);
							printf("\t\t\t\t\t\tadd_to_buffer(cbuf, c_status, \"%s enabled\", \"/set: \");\n", ents[i].set_msg);
							printf("\t\t\t\t\telse\n");
							printf("\t\t\t\t\t\tadd_to_buffer(cbuf, c_status, \"%s disabled\", \"/set: \");\n", ents[i].set_msg);
						break;
						case LEVEL:
							printf("\t\t\t\t\tif(%s)\n", ents[i].cname);
							printf("\t\t\t\t\t{\n");
							printf("\t\t\t\t\t\tchar lmsg[%u];\n", strlen(ents[i].set_msg)+32);
							printf("\t\t\t\t\t\tsprintf(lmsg, \"%s level %%u enabled\", %s);\n", ents[i].set_msg, ents[i].cname);
							printf("\t\t\t\t\t\tadd_to_buffer(cbuf, c_status, lmsg, \"/set: \");\n");
							printf("\t\t\t\t\t}\n");
							printf("\t\t\t\t\telse\n");
							printf("\t\t\t\t\t\tadd_to_buffer(cbuf, c_status, \"%s disabled\", \"/set: \");\n", ents[i].set_msg);
						break;
						case SET:
							printf("\t\t\t\t\tchar smsg[%u];\n", strlen(ents[i].set_msg)+24);
							printf("\t\t\t\t\tsprintf(smsg, \"%s set to %%u\", %s);\n", ents[i].set_msg, ents[i].cname);
							printf("\t\t\t\t\tadd_to_buffer(cbuf, c_status, smsg, \"/set: \");\n");
						break;
					}
					printf("\t\t\t\t}\n");
					if((ents[i].set_type==BOOLEAN)||(ents[i].set_type==LEVEL))
					{
						printf("\t\t\t\telse if(strcmp(opt, \"no-%s\")==0)\n", ents[i].set_name);
						printf("\t\t\t\t{\n");
						printf("\t\t\t\t\t%s=0;\n", ents[i].cname);
						printf("\t\t\t\t\t\tadd_to_buffer(cbuf, c_status, \"%s disabled\", \"/set: \");\n", ents[i].set_msg);
						printf("\t\t\t\t}\n");
					}
				}
			break;
			case 6:
				printf("\t\t\tif(strcmp(cmd, \"%s\")==0) need=false;\n", ents[i].rc_name);
				if((ents[i].set_type==BOOLEAN)||(ents[i].set_type==LEVEL))
					printf("\t\t\tif(strcmp(cmd, \"no-%s\")==0) need=false;\n", ents[i].rc_name);
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
	}
	
}

char * fgetl(FILE *fp)
{
	char * lout;
	int l,i;
	init_char(&lout, &l, &i);
	signed int c;
	while(!feof(fp))
	{
		c=fgetc(fp);
		if((c==EOF)||(c=='\n'))
			break;
		if(c!=0)
		{
			append_char(&lout, &l, &i, c);
		}
	}
	return(lout);
}

void append_char(char **buf, int *l, int *i, char c)
{
	if(!((c==0)||(c==EOF)))
	{
		if(*buf)
		{
			(*buf)[(*i)++]=c;
		}
		else
		{
			init_char(buf, l, i);
			append_char(buf, l, i, c);
		}
		char *nbuf=*buf;
		if((*i)>=(*l))
		{
			*l=*i*2;
			nbuf=(char *)realloc(*buf, *l);
		}
		if(nbuf)
		{
			*buf=nbuf;
			(*buf)[*i]=0;
		}
		else
		{
			free(*buf);
			init_char(buf, l, i);
		}
	}
}

void init_char(char **buf, int *l, int *i)
{
	*l=80;
	*buf=(char *)malloc(*l);
	(*buf)[0]=0;
	*i=0;
}
