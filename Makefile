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
	-mkdir quirc_$(VERSION)_src
	for p in $$(ls); do cp $$p quirc_$(VERSION)_src/$$p; done;
	cp -r dist quirc_$(VERSION)_src/dist
	-rm quirc_$(VERSION)_src/*.tar.gz
	rm quirc_$(VERSION)_src/*.o
	rm quirc_$(VERSION)_src/quirc
	mv quirc_$(VERSION)_src/distMakefile quirc_$(VERSION)_src/Makefile
	tar -czf quirc_$(VERSION)_src.tar.gz quirc_$(VERSION)_src/
	rm -r quirc_$(VERSION)_src

