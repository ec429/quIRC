# Makefile for quIRC

CC ?= gcc
CFLAGS ?= -Wall
AWK ?= gawk
VERSION := `git describe --tags`
PREFIX ?= /usr/local
LIBS := ttyraw.o ttyesc.o irc.o bits.o colour.o buffer.o names.o config.o input.o
INCLUDE := ttyraw.h ttyesc.h irc.h bits.h colour.h buffer.h names.h config.h input.h numeric.h version.h

all: quirc

install: all
	install -D quirc $(PREFIX)/bin/quirc

quirc: quirc.c $(LIBS) $(INCLUDE)
	$(CC) $(CFLAGS) -o quirc quirc.c $(LIBS)

# funky make cleverness to generate object files; a %.o /always/ depends on its %.h as well as its %.c

%.o: %.c %.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

irc.o: irc.c irc.h bits.h buffer.h colour.h config.h

bits.o: bits.c bits.h ttyesc.h colour.h config.h

colour.o: colour.c colour.h c_init.c ttyesc.h

buffer.o: buffer.c buffer.h ttyesc.h colour.h bits.h names.h text.h

config.o: config.c config.h bits.h colour.h text.h version.h

input.o: input.c input.h ttyesc.h names.h buffer.h

c_init.c: colour.d c_init.awk
	$(AWK) -f c_init.awk colour.d > c_init.c

version.h:
	./gitversion

# version.h is phony because, although the file exists, we always want to update it
.PHONY: version.h

dist: all
	-mkdir quirc_$(VERSION)
	for p in $$(ls); do cp $$p quirc_$(VERSION)/$$p; done;
	rm quirc_$(VERSION)/*.tar.gz
	tar -cvvf quirc_$(VERSION).tar quirc_$(VERSION)/
	gzip quirc_$(VERSION).tar
	rm -r quirc_$(VERSION)

