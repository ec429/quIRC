# Makefile for quIRC

VERSION := `git describe --tags`

include distMakefile

FORCE:
version.h: FORCE
	./gitversion

dist: all doc
	-mkdir quirc_$(VERSION)
	for p in $$(ls); do cp $$p quirc_$(VERSION)/$$p; done;
	cp -r dist quirc_$(VERSION)/dist
	-rm quirc_$(VERSION)/*.tar.gz
	mv quirc_$(VERSION)/distMakefile quirc_$(VERSION)/Makefile
	tar -czf quirc_$(VERSION).tar.gz quirc_$(VERSION)/
	rm -r quirc_$(VERSION)

dists: c_init.c config.c config.h version.h keymod.h keymap.c doc
	-mkdir quirc_$(VERSION).src
	for p in $$(ls); do cp $$p quirc_$(VERSION).src/$$p; done;
	cp -r dist quirc_$(VERSION).src/dist
	-rm quirc_$(VERSION).src/*.tar.gz
	rm quirc_$(VERSION).src/*.o
	rm quirc_$(VERSION).src/quirc
	rm quirc_$(VERSION).src/genconfig
	rm quirc_$(VERSION).src/genkeymap
	mv quirc_$(VERSION).src/distMakefile quirc_$(VERSION).src/Makefile
	tar -czf quirc_$(VERSION).src.tar.gz quirc_$(VERSION).src/
	rm -r quirc_$(VERSION).src

