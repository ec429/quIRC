#include "logging.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bits.h"

int log_add_plain(FILE *logf, mtype lm, prio lq, char lp, bool ls, const char *lt, const char *ltag, time_t ts);
int log_add_symbolic(FILE *logf, mtype lm, prio lq, char lp, bool ls, const char *lt, const char *ltag, time_t ts);
void safeprint(FILE *logf, const char *text, bool escape_spaces);

int log_init(FILE *logf, logtype logt)
{
	if(!logf) return(1);
	time_t now=time(NULL);
	switch(logt)
	{
		case LOGT_PLAIN:;
			char stamp[40];
			struct tm *td=gmtime(&now);
			strftime(stamp, 40, "[%H:%M:%S]", td);
			fprintf(logf, "* Started PLAIN logging at %s\n", stamp);
			fflush(logf);
			return(0);
		case LOGT_SYMBOLIC:
			fprintf(logf, "u+%lld LOGSTART\n", (signed long long)now);
			fflush(logf);
			return(0);
		default:
			return(2);
	}
}

int log_add(FILE *logf, logtype logt, mtype lm, prio lq, char lp, bool ls, const char *lt, const char *ltag, time_t ts)
{
	if(!logf) return(1);
	int e;
	switch(logt)
	{
		case LOGT_PLAIN:
			e=log_add_plain(logf, lm, lq, lp, ls, lt, ltag, ts);
			fflush(logf);
			return(e);
		case LOGT_SYMBOLIC:
			e=log_add_symbolic(logf, lm, lq, lp, ls, lt, ltag, ts);
			fflush(logf);
			return(e);
		default:
			return(2);
	}
}

int log_add_plain(FILE *logf, mtype lm, __attribute__((unused)) prio lq, char lp, __attribute__((unused)) bool ls, const char *lt, const char *ltag, time_t ts)
{
	char stamp[40];
	struct tm *td=gmtime(&ts);
	strftime(stamp, 40, "[%H:%M:%S] ", td);
	char *tag=strdup(ltag?ltag:"");
	switch(lm)
	{
		case MSG:
		{
			char mk[6]="<%s> ";
			if(lp)
				mk[0]=mk[3]=lp;
			crush(&tag, 16);
			char *ntag=mktag(mk, tag);
			free(tag);
			tag=ntag;
		}
		break;
		case NOTICE:
		{
			if(*tag)
			{
				crush(&tag, 16);
				char *ntag=mktag("(from %s) ", tag);
				free(tag);
				tag=ntag;
			}
		}
		break;
		case PREFORMAT:
		break;
		case ACT:
		{
			crush(&tag, 16);
			char *ntag=mktag("* %s ", tag);
			free(tag);
			tag=ntag;
		}
		break;
		case QUIT_PREFORMAT:
		break;
		case JOIN:
		case PART:
		case QUIT:
		case NICK:
		{
			crush(&tag, 16);
			char *ntag=mktag("=%s= ", tag);
			free(tag);
			tag=ntag;
		}
		break;
		case MODE:
		break;
		case STA:
			free(tag);
			return(0);
		break;
		case ERR:
		break;
		case UNK:
		break;
		case UNK_NOTICE:
			if(*tag)
			{
				crush(&tag, 16);
				char *ntag=mktag("(from %s) ", tag);
				free(tag);
				tag=ntag;
			}
		break;
		case UNN:
		break;
		default:
		break;
	}
	fprintf(logf, "%s%s%s\n", stamp, tag, lt);
	free(tag);
	return(0);
}

int log_add_symbolic(FILE *logf, mtype lm, prio lq, char lp, bool ls, const char *lt, const char *ltag, time_t ts)
{
	if(!logf) return(1);
	fprintf(logf, "u+%lld %s %s %c %c ", (signed long long)ts, mtype_name(lm), prio_name(lq), lp?lp:'0', ls?'>':'<');
	safeprint(logf, ltag, true);
	fputc(' ', logf);
	safeprint(logf, lt, false);
	fputc('\n', logf);
	return(0);
}

void safeprint(FILE *logf, const char *text, bool es)
{
	if(!logf) return;
	if(!text) return;
	while(*text)
	{
		if((!isprint(*text)) || (es&&isspace(*text)))
			fprintf(logf, "\\%03o", *text);
		else
			fputc(*text, logf);
		text++;
	}
}
