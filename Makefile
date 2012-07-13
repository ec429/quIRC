# Makefile for quIRC

CC ?= gcc
OPTFLAGS := -g
CFLAGS = -Wall -Wextra -Werror -pedantic -std=gnu99 -D_GNU_SOURCE $(OPTFLAGS)
AWK := gawk
VERSION := `git describe --tags`
PREFIX := /usr/local
LIBS_ASYNCH_NL := -lanl
OPTLIBS = $(LIBS_ASYNCH_NL)
LIBS = -lm -lncurses $(OBJS) $(OPTLIBS)
OBJS := ttyraw.o ttyesc.o irc.o bits.o strbuf.o colour.o buffer.o names.o config.o input.o logging.o types.o
INCLUDE := $(OBJS:.o=.h) quirc.h version.h osconf.h

-include config.mak

all: quirc doc

install: all doc
	install -D -m0755 quirc $(PREFIX)/bin/quirc
	install -D -m0644 quirc.1 $(PREFIX)/man/man1/quirc.1
	install -D -m0644 readme.htm $(PREFIX)/share/doc/quirc/readme.htm
	install -D -m0644 config_ref.htm $(PREFIX)/share/doc/quirc/config_ref.htm
	install -D -m0644 tutorial.htm $(PREFIX)/share/doc/quirc/tutorial.htm

uninstall:
	-rm $(PREFIX)/bin/quirc
	-rm $(PREFIX)/man/man1/quirc.1
	-rm $(PREFIX)/share/doc/quirc/readme.htm
	-rm $(PREFIX)/share/doc/quirc/config_ref.htm
	-rm $(PREFIX)/share/doc/quirc/tutorial.htm

quirc: quirc.c $(OBJS) $(INCLUDE)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBS) $(DEFINES)

quirc.h: config.h version.h
	touch quirc.h

clean:
	-rm -f *.o quirc genconfig genkeymap

realclean: clean
	-rm -f c_init.c README version.h config_* keymap.c keymod.h quirc.1

doc: README config_ref.htm quirc.1

README: readme.htm
	-sed -e "s/&apos;/'/g" -e "s/&quot;/\"/g" < readme.htm | html2text -nobs -o README

# warning, this explodes if PREFIX contains a !
quirc.1: man.in
	echo ".\\\"\n.\\\"This man page is automatically generated from man.in by a sedscript; edit man.in, not this file, and 'make doc' to apply the changes.">quirc.1
	sed -e "s!\$$PREFIX!$(PREFIX)!g" < man.in >> quirc.1

# funky make cleverness to generate object files; a %.o /always/ depends on its %.h as well as its %.c

%.o: %.c %.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@ $(DEFINES)

ttyesc.o: ttyesc.c ttyesc.h bits.h

irc.o: irc.c irc.h bits.h buffer.h colour.h names.h numeric.h osconf.h

irc.h: config.h
	touch irc.h

bits.o: bits.c bits.h ttyesc.h colour.h

bits.h: config.h strbuf.h
	touch bits.h

colour.o: colour.c colour.h c_init.c ttyesc.h

buffer.o: buffer.c buffer.h ttyesc.h colour.h bits.h names.h text.h irc.h version.h input.h logging.h

buffer.h: config.h version.h logging.h
	touch buffer.h

config.o: config.c config.h names.h bits.h colour.h text.h version.h

config.c: config_check.c config_def.c config_need.c config_rcread.c config_pargs.c config_help.c keymap.c
	touch config.c

config.h: config_globals.h version.h keymod.h
	touch config.h

config_%: config.cdl genconfig
	./genconfig $@ < config.cdl > $@ || (rm $@ && false)

genconfig: genconfig.c strbuf.h strbuf.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LDFLAGS) strbuf.o -o $@

input.h: keymod.h
	touch input.h

input.o: input.c input.h ttyesc.h names.h buffer.h irc.h bits.h config.h logging.h

input.c: config_set.c
	touch input.c

logging.o: types.h bits.h

names.o: names.c names.h buffer.h irc.h

script.o: script.c script.h bits.h buffer.h

c_init.c: colour.d c_init.awk
	$(AWK) -f c_init.awk colour.d > c_init.c

genkeymap: genkeymap.c strbuf.h strbuf.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LDFLAGS) strbuf.o -o $@

keymod.h: keys genkeymap
	./genkeymap h < keys > $@ || (rm $@ && false)

keymap.c: keys genkeymap
	./genkeymap c < keys > $@ || (rm $@ && false)

FORCE:
version.h: FORCE
	./gitversion

dist: all doc
	-mkdir quirc_$(VERSION)
	for p in $$(ls); do cp $$p quirc_$(VERSION)/$$p; done;
	-rm quirc_$(VERSION)/*.tar.gz
	mv quirc_$(VERSION)/distMakefile quirc_$(VERSION)/Makefile
	tar -czf quirc_$(VERSION).tar.gz quirc_$(VERSION)/
	rm -r quirc_$(VERSION)

dists: c_init.c config.c config.h version.h keymod.h keymap.c doc
	-mkdir quirc_$(VERSION)_src
	for p in $$(ls); do cp $$p quirc_$(VERSION)_src/$$p; done;
	-rm quirc_$(VERSION)_src/*.tar.gz
	rm quirc_$(VERSION)_src/*.o
	rm quirc_$(VERSION)_src/quirc
	mv quirc_$(VERSION)_src/distMakefile quirc_$(VERSION)_src/Makefile
	tar -czf quirc_$(VERSION)_src.tar.gz quirc_$(VERSION)_src/
	rm -r quirc_$(VERSION)_src

