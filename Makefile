# Makefile for quIRC

CC ?= gcc
CFLAGS ?= -Wall
AWK ?= gawk
VERSION := `git describe --tags`
PREFIX ?= /usr/local
LIBS := ttyraw.o ttyesc.o irc.o bits.o colour.o buffer.o names.o config.o
INCLUDE := ttyraw.h ttyesc.h irc.h bits.h colour.h buffer.h names.h config.h numeric.h version.h

all: quirc

install: all
	install -D quirc $(PREFIX)/bin/quirc

quirc: quirc.c $(LIBS) $(INCLUDE)
	$(CC) $(CFLAGS) -o quirc quirc.c $(LIBS)

# TODO use funky make cleverness for these rules as they're all basically the same

ttyraw.o: ttyraw.c ttyraw.h
	$(CC) $(CFLAGS) -o ttyraw.o -c ttyraw.c

ttyesc.o: ttyesc.c ttyesc.h
	$(CC) $(CFLAGS) -o ttyesc.o -c ttyesc.c

irc.o: irc.c irc.h bits.h
	$(CC) $(CFLAGS) -o irc.o -c irc.c

bits.o: bits.c bits.h ttyesc.h colour.h
	$(CC) $(CFLAGS) -o bits.o -c bits.c

colour.o: colour.c colour.h c_init.c ttyesc.h
	$(CC) $(CFLAGS) -o colour.o -c colour.c

buffer.o: buffer.c buffer.h ttyesc.h colour.h bits.h names.h
	$(CC) $(CFLAGS) -o buffer.o -c buffer.c

names.o: names.c names.h
	$(CC) $(CFLAGS) -o names.o -c names.c

config.o: config.c config.h bits.h colour.h text.h version.h
	$(CC) $(CFLAGS) -o config.o -c config.c

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

