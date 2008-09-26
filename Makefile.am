## Process this file with automake to produce Makefile.in

SUBDIRS = src

rpm: clean
	@(unset CDPATH ; $(MAKE) dist && cp $(distdir).tar.gz /usr/src/redhat/SOURCES/ && rpmbuild -ba libvirt-qpid.spec)

install-data-local: install-init
uninstall-local:: uninstall-init

install-init: libvirt-qpid.init
	mkdir -p $(DESTDIR)$(sysconfdir)/rc.d/init.d
	$(INSTALL_SCRIPT) libvirt-qpid.init \
	  $(DESTDIR)$(sysconfdir)/rc.d/init.d/libvirt-qpid

uninstall-init:
	rm -f $(DESTDIR)$(sysconfdir)/rc.d/init.d/libvirt-qpid

libvirt-qpid.init: libvirt-qpid.init.in
	sed						\
	    -e s!\@localstatedir\@!@localstatedir@!g	\
	    -e s!\@sbindir\@!@sbindir@!g		\
	    -e s!\@sysconfdir\@!@sysconfdir@!g		\
	    < $< > $@-t
	chmod a+x $@-t
	mv $@-t $@

EXTRA_DIST = 	autogen.sh \
		libvirt-qpid.init.in

CLEANFILES = 	libvirt-qpid.init

