#pragma once
/*
	quIRC - simple terminal-based IRC client
	Copyright (C) 2010-13 Edward Cree

	See quirc.c for license information
	ctbuf: coloured text buffers
*/

#include "colour.h"

typedef struct
{
	colour c;
	char d;
}
ctchar;

void ct_init_char(ctchar **buf, size_t *l, size_t *i);
void ct_append_char(ctchar **buf, size_t *l, size_t *i, char d);
void ct_append_char_c(ctchar **buf, size_t *l, size_t *i, colour c, char d);
void ct_append_str(ctchar **buf, size_t *l, size_t *i, const char *str);
void ct_append_str_c(ctchar **buf, size_t *l, size_t *i, colour c, const char *str);
void ct_putchar(ctchar a);
void ct_puts(const ctchar *buf);
void ct_putsn(const ctchar *buf, size_t n);
