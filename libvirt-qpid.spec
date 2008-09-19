Summary: QPid QMF interface to Libvirt
Name: libvirt-qpid
AutoReq: no
Version: 0.1.1
Release: 1
Source: libvirt-qpid-%{version}.tar.gz
License: LGPL
Group: Applications/System
Requires: qmf >= 0.3.696027
Requires: qpidd >= 3.696027
Requires: qpidc >= 0.3.696027
Requires: libvirt >= 0.4.4
Requires(post):  /sbin/chkconfig
Requires(preun): /sbin/chkconfig
Requires(preun): /sbin/service
BuildRequires: qpidd-devel >= 3.696027
BuildRequires: qpidc-devel >= 3.696027
BuildRequires: libvirt-devel >= 0.4.4
BuildRequires: qmf-devel >= 0.3.696027
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
#Url: http://libvirt.org/html/libvirt-qpid

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
(cd src && make)

%install
(cd src && make install)

%post
#/sbin/chkconfig --add libvirt-qpid
#/sbin/service libvirt-qpid condrestart

%preun
#if [ $1 = 0 ]; then
    #/sbin/service libvirt-qpid stop >/dev/null 2>&1 || :
    #chkconfig --del libvirt-qpid
#fi

%postun
#if [ "$1" -ge "1" ]; then
    #/sbin/service libvirt-qpid condrestart >/dev/null 2>&1 || :
    #/sbin/service httpd condrestart >/dev/null 2>&1 || :
#fi

%clean
test "x$RPM_BUILD_ROOT" != "x" && rm -rf $RPM_BUILD_ROOT

%files

%defattr(644,root,root)
/usr/share/libvirt-qpid/libvirt-schema.xml

%defattr(755,root,root)
%{_sbindir}/libvirt-qpid

%doc AUTHORS CHANGELOG README COPYING


%changelog

* Fri Sep 19 12:47:16 PDT 2008
- Initial packaging.


