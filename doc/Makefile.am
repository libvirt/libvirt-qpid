XSLTPROC = /usr/bin/xsltproc

WEB_PAGES = index.html \
            architecture.html \
            releases.html

EXTRA_DIST = site.xsl libvirt-qpid.html $(WEB_PAGES)

$(WEB_PAGES): libvirt-qpid.html site.xsl
	-@(if [ -x $(XSLTPROC) ]; then \
	   $(XSLTPROC) --nonet --html $(top_srcdir)/doc/site.xsl $(top_srcdir)/doc/libvirt-qpid.html > index.html; \
	   fi);

#install-data-local:
#	$(mkinstalldirs) $(DESTDIR)$(HTML_DIR)
#	@INSTALL@ -m 0644 $(srcdir)/*.html $(DESTDIR)$(HTML_DIR)

all: $(WEB_PAGES)
