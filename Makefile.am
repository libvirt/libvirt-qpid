## Process this file with automake to produce Makefile.in

SUBDIRS = src doc

rpm: clean
	@(unset CDPATH ; $(MAKE) dist && cp $(distdir).tar.gz \
	  $(HOME)/rpmbuild/SOURCES/ && rpmbuild -ba libvirt-qpid.spec)

install-data-local: install-init
uninstall-local:: uninstall-init

install-init: libvirt-qpid.init
	mkdir -p $(DESTDIR)$(sysconfdir)/rc.d/init.d
	$(INSTALL_SCRIPT) libvirt-qpid.init \
	  $(DESTDIR)$(sysconfdir)/rc.d/init.d/libvirt-qpid
	mkdir -p $(DESTDIR)$(sysconfdir)/sysconfig
	$(INSTALL_SCRIPT) libvirt-qpid.sysconf \
	  $(DESTDIR)$(sysconfdir)/sysconfig/libvirt-qpid

uninstall-init:
	rm -f $(DESTDIR)$(sysconfdir)/rc.d/init.d/libvirt-qpid \
		$(DESTDIR)$(sysconfdir)/sysconfig/libvirt-qpid

libvirt-qpid.init: libvirt-qpid.init.in
	sed						\
	    -e s!\@localstatedir\@!@localstatedir@!g	\
	    -e s!\@sbindir\@!@sbindir@!g		\
	    -e s!\@sysconfdir\@!@sysconfdir@!g		\
	    < $< > $@-t
	chmod a+x $@-t
	mv $@-t $@

EXTRA_DIST = 	autogen.sh \
		libvirt-qpid.init.in \
		libvirt-qpid.sysconf

CLEANFILES = 	libvirt-qpid.init

