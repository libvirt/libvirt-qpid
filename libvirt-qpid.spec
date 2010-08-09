Summary: QPid QMF interface to Libvirt
Name: libvirt-qpid
Version: 0.2.20
Release: 1%{?dist}
Source: libvirt-qpid-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
License: LGPLv2+
Group: Applications/System
Requires: libxml2 >= 2.7.1
Requires: qmf >= 0.5.790661
Requires: qpid-cpp-client >= 0.5.790661
Requires: libvirt >= 0.4.4
Requires(post):  /sbin/chkconfig
Requires(preun): /sbin/chkconfig
Requires(preun): initscripts
BuildRequires: qpid-cpp-client-devel >= 0.5.790661
BuildRequires: libxml2-devel >= 2.7.1
BuildRequires: libvirt-devel >= 0.5.0
BuildRequires: qmf-devel >= 0.5.790661
Url: http://libvirt.org/qpid

%description

libvirt-qpid provides an interface with libvirt using QMF (qpid modeling
framework) which utilizes the AMQP protocol.  The Advanced Message Queuing
Protocol (AMQP) is an open standard application layer protocol providing
reliable transport of messages.

QMF provides a modeling framework layer on top of qpid (which implements 
AMQP).  This interface allows you to manage hosts, domains, pools etc. as
a set of objects with properties and methods.

%prep
%setup -q

%build
%configure
make

%install
rm -rf %{buildroot}
%makeinstall

%post
/sbin/chkconfig --add libvirt-qpid --level -
/sbin/service libvirt-qpid condrestart

%preun
if [ $1 = 0 ]; then
    /sbin/service libvirt-qpid stop >/dev/null 2>&1 || :
    chkconfig --del libvirt-qpid
fi

%postun
if [ "$1" -ge "1" ]; then
    /sbin/service libvirt-qpid condrestart >/dev/null 2>&1 || :
fi

%clean
test "x$RPM_BUILD_ROOT" != "x" && rm -rf $RPM_BUILD_ROOT

%files

%defattr(644, root, root, 755)
%dir %{_datadir}/libvirt-qpid/
%{_datadir}/libvirt-qpid/libvirt-schema.xml

%attr(755, root, root) %{_sbindir}/libvirt-qpid
%attr(755, root, root) %{_sysconfdir}/rc.d/init.d/libvirt-qpid
%config(noreplace) %{_sysconfdir}/sysconfig/libvirt-qpid

%doc AUTHORS COPYING


%changelog
* Tue Jul 6 Ian Main <imain@redhat.com> - 0.2.22-1
- Init script fixes.

* Mon Jun 28 Ian Main <imain@redhat.com> - 0.2.21-1
- Add indexes to various attributes so they are unique in qpidd etc.

* Tue May 26 Ian Main <imain@redhat.com> - 0.2.20-1
- #include <string.h> in PoolWrap.cpp.

* Tue May 25 Ian Main <imain@redhat.com> - 0.2.19-1
- Update for changed QMF connection API.

* Mon Feb 22 2010 Ian Main <imain@redhat.com> - 0.2.18-1
- Update for changed qpid pkg names.

* Thu Dec 10 2009 Ian Main <imain@redhat.com> - 0.2.17-3
- Woops, don't need -lqmfengine.

* Thu Nov 05 2009 Ian Main <imain@redhat.com> - 0.2.17-2
- Fix libraries for new qpidc in src.

* Fri Oct 02 2009 Kevin Kofler <Kevin@tigcc.ticalc.org> - 0.2.17-1
- Rebuild for new qpidc.

* Mon Jul 27 2009 Arjun Roy <arroy@redhat.com> - 0.2.17.0
- Fixed a bug related to updating of cached pool sources.

* Fri Jun 05 2009 Ian Main <imain@redhat.com> - 0.2.16-5
- Bump for new build.

* Fri Jun 05 2009 Ian Main <imain@redhat.com> - 0.2.16-2
- Spec fixes.

* Fri Jun 05 2009 Ian Main <imain@redhat.com> - 0.2.16-1
- Cache calls to get storage XML as they take a LONG time.  This
  greatly speeds up volume enumeration.

* Thu May 28 2009 Ian Main <imain@redhat.com> - 0.2.16-1
- Cache calls to get storage XML as they take a LONG time.  This
  greatly speeds up volume enumeration.

* Thu May 28 2009 Ian Main <imain@redhat.com> - 0.2.15-1
- Fix migration bug.

* Thu May 07 2009 Ian Main <imain@redhat.com> - 0.2.14-4
- Bump release for F11.

* Thu May 07 2009 Ian Main <imain@redhat.com> - 0.2.14-3
- Update requires for latest version of qpid.

* Thu May 07 2009 Ian Main <imain@redhat.com> - 0.2.14-2
- Rebuild for new qpid.

* Thu May 07 2009 Ian Main <imain@redhat.com> - 0.2.14-1
- Rebuild for new qpid.

* Thu Mar 26 2009 Ian Main <imain@redhat.com> - 0.2.14-0
- Added refresh method to storage pools.

* Thu Mar 26 2009 Ian Main <imain@redhat.com> - 0.2.13-1
- Add throws to constructors in case one of the libvirt calls
  fails.  Pretty sure this was the cause of some segfaults.

* Thu Mar 26 2009 Ian Main <imain@redhat.com> - 0.2.12-3
- Added dist to release version.

* Wed Feb 25 2009 Ian Main <imain@redhat.com> - 0.2.12-2
- Fixed permissions in specfile.

* Wed Feb 25 2009 Ian Main <imain@redhat.com> - 0.2.12-1
- Fixed various specfile issues.

* Tue Feb 03 2009 Ian Main <imain@redhat.com> - 0.2.12-0
- Added parentVolume to support LVM parent recognition.

* Fri Jan 23 2009 Ian Main <imain@redhat.com> - 0.2.10-0
- Added support for gssapi.

* Fri Dec 12 2008 Ian Main <imain@redhat.com> - 0.2.4-0
- Added 'findStoragePoolSources' method.

* Thu Dec 4 2008 Ian Main <imain@redhat.com> - 0.2.3-0
- Added 'build' method to storage pool.
- Build against newer libvirt and qpid.

* Wed Nov 20 2008 Ian Main <imain@redhat.com> - 0.2.2-0
- Change update interval to 3 seconds, update version.

* Wed Nov 19 2008 Ian Main <imain@redhat.com> - 0.2.2-0
- Rebase to newer qpid.

* Thu Oct 30 2008 Ian Main <imain@redhat.com> - 0.2.1-0
- Use lstr for xml descriptions.  This lets you have greater than
  255 characters in the string.
- Fix bug in calling of getXMLDesc.

* Wed Oct 15 2008 Ian Main <imain@redhat.com> - 0.2.0-0
- API changed to camel case.
- Return libvirt error codes.
- Reconnect on libvirt disconnect.
- Implement node info.
- New release.

* Wed Oct 1 2008 Ian Main <imain@redhat.com> - 0.1.3-0
- Bugfixes, memory leaks fixed etc.

* Tue Sep 30 2008 Ian Main <imain@redhat.com> - 0.1.2-0
- Updated spec to remove qpidd requirement.
- Added libvirt-qpid sysconfig file.

* Fri Sep 26 2008 Ian Main <imain@redhat.com> - 0.1.2-0
- Setup daemonization and init scripts.
- Added getopt for command line parsing.

* Fri Sep 19 2008 Ian Main <imain@redhat.com> - 0.1.1-0
- Initial packaging.



