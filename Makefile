# Makefile for quIRC

CC ?= gcc
CFLAGS ?= -Wall

all: quirc

quirc: quirc.c ttyraw.o ttyraw.h ttyesc.o ttyesc.h irc.o irc.h bits.o bits.h colour.o colour.h numeric.h
	./gitversion
	$(CC) $(CFLAGS) -o quirc quirc.c ttyraw.o ttyesc.o irc.o bits.o colour.o

ttyraw.o: ttyraw.c ttyraw.h
	$(CC) $(CFLAGS) -o ttyraw.o -c ttyraw.c

ttyesc.o: ttyesc.c ttyesc.h
	$(CC) $(CFLAGS) -o ttyesc.o -c ttyesc.c

irc.o: irc.c irc.h
	$(CC) $(CFLAGS) -o irc.o -c irc.c

bits.o: bits.c bits.h
	$(CC) $(CFLAGS) -o bits.o -c bits.c

colour.o: colour.c colour.h
	$(CC) $(CFLAGS) -o colour.o -c colour.c

dist: all
	VERSION = `git describe`
	mkdir quirc_$(VERSION)
	cp * quirc_$(VERSION)
	rm quirc_$(VERSION)/.quirc*
	tar -cvvf quirc_$(VERSION)
	gzip quirc_$(VERSION).tar
	rm -r quirc_$(VERSION)

