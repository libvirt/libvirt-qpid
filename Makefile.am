## Process this file with automake to produce Makefile.in

SUBDIRS = src

rpm: clean
	@(unset CDPATH ; $(MAKE) dist && cp $(distdir).tar.gz /usr/src/redhat/SOURCES/ && rpmbuild -ba libvirt-qpid.spec)

EXTRA_DIST = autogen.sh
