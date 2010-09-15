# Makefile for quIRC

CC ?= gcc
CFLAGS ?= -Wall
AWK ?= gawk
VERSION := `git describe --tags`
PREFIX ?= /usr/local
LIBS := ttyraw.o ttyesc.o irc.o bits.o colour.o buffer.o names.o config.o input.o
INCLUDE := ttyraw.h ttyesc.h irc.h bits.h colour.h buffer.h names.h config.h input.h version.h
DEFINES ?= -DHAVE_DEBUG

all: quirc

install: all
	install -D quirc $(PREFIX)/bin/quirc

uninstall:
	rm $(PREFIX)/bin/quirc

quirc: quirc.c $(LIBS) $(INCLUDE)
	$(CC) $(CFLAGS) -o quirc quirc.c $(LIBS) $(DEFINES)

mtrace: quirc-mtrace

quirc-mtrace: quirc.c $(LIBS) $(INCLUDE)
	$(CC) $(CFLAGS) -o quirc-mtrace quirc.c $(LIBS) -g -DUSE_MTRACE $(DEFINES)

clean:
	rm *.o quirc

realclean: clean
	rm c_init.c README version.h

doc: README

README: readme.htm
	html2text -nobs -o README < readme.htm

# funky make cleverness to generate object files; a %.o /always/ depends on its %.h as well as its %.c

%.o: %.c %.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@ $(DEFINES)

ttyesc.o: bits.h

irc.o: irc.c irc.h bits.h buffer.h colour.h config.h names.h numeric.h

bits.o: bits.c bits.h ttyesc.h colour.h config.h

colour.o: colour.c colour.h c_init.c ttyesc.h

buffer.o: buffer.c buffer.h ttyesc.h colour.h bits.h names.h text.h irc.h config.h version.h input.h

config.o: config.c config.h names.h bits.h colour.h text.h version.h

input.o: input.c input.h ttyesc.h names.h buffer.h irc.h

names.o: names.c names.h buffer.h

c_init.c: colour.d c_init.awk
	$(AWK) -f c_init.awk colour.d > c_init.c

# version is touched by a git-commit hook
version.h: version
	./gitversion

dist: all doc
	-mkdir quirc_$(VERSION)
	for p in $$(ls); do cp $$p quirc_$(VERSION)/$$p; done;
	rm quirc_$(VERSION)/*.tar.gz
	tar -cvvf quirc_$(VERSION).tar quirc_$(VERSION)/
	gzip quirc_$(VERSION).tar
	rm -r quirc_$(VERSION)

