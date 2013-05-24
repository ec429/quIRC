/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	genkeymap: generate keymapping code from data
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "strbuf.h"

typedef struct
{
	char *name;
	char *mod;
}
keymod;

int main(int argc, char **argv)
{
	if(argc!=2)
	{
		fprintf(stderr, "Usage: genkeymap {c|h}\n");
		return(EXIT_FAILURE);
	}
	int mode=0;
	if(strcmp(argv[1], "c")==0)
		mode=1;
	else if(strcmp(argv[1], "h")==0)
		mode=2;
	if(!mode)
	{
		fprintf(stderr, "Usage: genkeymap {c|h}\n");
		return(EXIT_FAILURE);
	}
	int nkeys=0;
	keymod *keys=NULL;
	while(!feof(stdin))
	{
		char *line=fgetl(stdin);
		if(line)
		{
			if((*line)&&(*line!='#'))
			{
				keymod new;
				new.name=strdup(strtok(line, " \t"));
				char *mod=strtok(NULL, "\t");
				if(*mod==':')
				{
					new.mod=strdup(mod);
				}
				else
				{
					off_t o=strlen(mod);
					if(o&1)
					{
						fprintf(stderr, "genkeymap: bad line (o&1) %s\t%s\n", new.name, mod);
						return(EXIT_FAILURE);
					}
					new.mod=malloc((o>>1)+1);
					for(int i=0;i<o;i+=2)
					{
						if(!(isxdigit(mod[i])&&isxdigit(mod[i+1])))
						{
							fprintf(stderr, "genkeymap: bad line (not hex) %s\t%s\n", new.name, mod);
							return(EXIT_FAILURE);
						}
						char buf[3];buf[0]=mod[i];buf[1]=mod[i+1];buf[2]=0;
						unsigned int c;
						if(sscanf(buf, "%x", &c)!=1)
						{
							fprintf(stderr, "genkeymap: bad line (sscanf) %s\t%s\n", new.name, mod);
							return(EXIT_FAILURE);
						}
						new.mod[i>>1]=c;
					}
					new.mod[o>>1]=0;
				}
				int n=nkeys++;
				(keys=realloc(keys, nkeys*sizeof(keymod)))[n]=new;
			}
			free(line);
		}
		else
			break;
	}
	switch(mode)
	{
		case 1:
			printf("#include <stdbool.h>\n\n");
			printf("int initkeys(void)\n{\n");
			printf("\tnkeys=%d;\n", nkeys);
			printf("\tkmap=malloc(nkeys*sizeof(keymod));\n");
			printf("\tif(!kmap) return(1);\n");
			for(int i=0;i<nkeys;i++)
			{
				printf("\tkmap[%d].name=\"%s\";\n", i, keys[i].name);
				if(keys[i].mod[0]==':')
					printf("\tif(%s) kmap[%d].mod=%s; else kmap[%d].mod=\"\";\n", keys[i].mod+1, i, keys[i].mod+1, i);
				else
				{
					printf("\tkmap[%d].mod=malloc(%zu);\n", i, strlen(keys[i].mod)+1);
					for(unsigned int j=0;j<=strlen(keys[i].mod);j++)
					{
						printf("\tkmap[%d].mod[%d]=%hhd;\n", i, j, keys[i].mod[j]);
					}
				}
			}
			printf("\treturn(0);\n");
			printf("}\n");
		break;
		case 2:
			printf("#pragma once\n\ntypedef struct\n{\n\tconst char *name;\n\tchar *mod;\n}\nkeymod;\n\n");
			for(int i=0;i<nkeys;i++)
			{
				printf("#define KEY_%s\t%d\n", keys[i].name, i);
			}
			printf("#define KEY_F(n)\t((int[12]){");
			for(int i=1;i<=12;i++)
			{
				printf("KEY_F%d,", i);
			}
			printf("}[n-1])\n");
		break;
	}
	return(0);
}
