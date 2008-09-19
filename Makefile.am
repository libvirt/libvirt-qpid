## Process this file with automake to produce Makefile.in

SUBDIRS = src

rpm: clean
	@(unset CDPATH ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.gz)

EXTRA_DIST = autogen.sh
