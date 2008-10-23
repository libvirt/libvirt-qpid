Summary: QPid QMF interface to Libvirt
Name: libvirt-qpid
Version: 0.2.0
Release: 3
Source: libvirt-qpid-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
License: LGPL
Group: Applications/System
Requires: qmf >= 0.3.696027
Requires: qpidc >= 0.3.696027
Requires: libvirt >= 0.4.4
Requires(post):  /sbin/chkconfig
Requires(preun): /sbin/chkconfig
Requires(preun): /sbin/service
BuildRequires: qpidc-devel >= 0.3.696027
BuildRequires: libvirt-devel >= 0.4.4
BuildRequires: qmf-devel >= 0.3.696027
ExclusiveArch: i386 x86_64
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
/sbin/chkconfig --add libvirt-qpid
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

%defattr(644,root,root)
%dir %{_datadir}/libvirt-qpid/
%{_datadir}/libvirt-qpid/libvirt-schema.xml

%defattr(755,root,root)
%{_sbindir}/libvirt-qpid
%{_sysconfdir}/rc.d/init.d/libvirt-qpid
%config(noreplace) %{_sysconfdir}/sysconfig/libvirt-qpid

%doc AUTHORS COPYING


%changelog

* Wed Oct 15 2008 Ian Main <imain@redhat.com - 0.2.0
- API changed to camel case.  
- Return libvirt error codes.
- Reconnect on libvirt disconnect.
- Implement node info.
- New release.

* Wed Oct 1 2008 Ian Main <imain@redhat.com - 0.1.3
- Bugfixes, memory leaks fixed etc.

* Tue Sep 30 2008 Ian Main <imain@redhat.com - 0.1.2
- Updated spec to remove qpidd requirement.
- Added libvirt-qpid sysconfig file.

* Fri Sep 26 2008 Ian Main <imain@redhat.com - 0.1.2
- Setup daemonization and init scripts.
- Added getopt for command line parsing.

* Fri Sep 19 2008 Ian Main <imain@redhat.com - 0.1.1
- Initial packaging.



