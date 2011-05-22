# Makefile for quIRC

CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -pedantic -std=gnu99 -g
AWK ?= gawk
VERSION := `git describe --tags`
PREFIX ?= /usr/local
LIBS := ttyraw.o ttyesc.o irc.o bits.o colour.o buffer.o names.o config.o input.o
INCLUDE := ttyraw.h ttyesc.h irc.h bits.h colour.h buffer.h names.h config.h input.h quirc.h version.h
DEFINES ?= -DHAVE_DEBUG

all: quirc doc

install: all
	sudo install -D quirc $(PREFIX)/bin/quirc

uninstall:
	rm $(PREFIX)/bin/quirc

quirc: quirc.c $(LIBS) $(INCLUDE)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBS) -lm $(DEFINES)

quirc.h: config.h version.h
	touch quirc.h

clean:
	-rm *.o quirc genconfig

realclean: clean
	-rm c_init.c README version.h config_*

doc: README config_ref.htm

README: readme.htm
	-sed -e "s/&apos;/'/g" -e "s/&quot;/\"/g" < readme.htm | html2text -nobs -o README

# funky make cleverness to generate object files; a %.o /always/ depends on its %.h as well as its %.c

%.o: %.c %.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@ $(DEFINES)

ttyesc.o: ttyesc.c ttyesc.h bits.h

irc.o: irc.c irc.h bits.h buffer.h colour.h names.h numeric.h

irc.h: config.h
	touch irc.h

bits.o: bits.c bits.h ttyesc.h colour.h

bits.h: config.h
	touch bits.h

colour.o: colour.c colour.h c_init.c ttyesc.h

buffer.o: buffer.c buffer.h ttyesc.h colour.h bits.h names.h text.h irc.h version.h input.h

buffer.h: config.h version.h
	touch buffer.h

config.o: config.c config.h names.h bits.h colour.h text.h version.h

config.c: config_check.c config_def.c config_need.c config_rcread.c config_pargs.c config_help.c
	touch config.c

config.h: config_globals.h version.h
	touch config.h

config_%: config.cdl genconfig
	./genconfig $@ < config.cdl > $@ || (rm $@ && false)

genconfig: genconfig.c

input.o: input.c input.h ttyesc.h names.h buffer.h irc.h bits.h

input.c: config_set.c
	touch input.c

names.o: names.c names.h buffer.h irc.h

script.o: script.c script.h bits.h buffer.h

c_init.c: colour.d c_init.awk
	$(AWK) -f c_init.awk colour.d > c_init.c

FORCE:
version.h: FORCE
	./gitversion
	if ! cmp version.h version.h2; then mv version.h2 version.h; fi
	-rm version.h2

dist: all doc
	-mkdir quirc_$(VERSION)
	for p in $$(ls); do cp $$p quirc_$(VERSION)/$$p; done;
	-rm quirc_$(VERSION)/*.tar.gz
	sed -i -e "s/\.\/gitversion/touch version.h/" -e "s/[g]it describe --tags/\.\/quirc -V 2>\&1 | col | head -n1 | grep -o \"quirc .*\" | tail -c+7/" quirc_$(VERSION)/Makefile
	tar -cvvf quirc_$(VERSION).tar quirc_$(VERSION)/
	gzip quirc_$(VERSION).tar
	rm -r quirc_$(VERSION)

dists: c_init.c doc
	-mkdir quirc_$(VERSION)_src
	for p in $$(ls); do cp $$p quirc_$(VERSION)_src/$$p; done;
	-rm quirc_$(VERSION)_src/*.tar.gz
	rm quirc_$(VERSION)_src/*.o
	rm quirc_$(VERSION)_src/quirc
	sed -i -e "s/\.\/gitversion/touch version.h/" -e "s/[g]it describe --tags/\.\/quirc -V 2>\&1 | col | head -n1 | grep -o \"quirc .*\" | tail -c+7/" quirc_$(VERSION)_src/Makefile
	tar -cvvf quirc_$(VERSION)_src.tar quirc_$(VERSION)_src/
	gzip quirc_$(VERSION)_src.tar
	rm -r quirc_$(VERSION)_src

