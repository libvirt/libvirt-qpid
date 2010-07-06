dnl Process this file with autoconf to produce a configure script.

AC_INIT(libvirt-qpid, 0.2.22)
AC_CONFIG_SRCDIR(src/NodeWrap.cpp)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])

LIBVIRT_QPID_MAJOR_VERSION=`echo $VERSION | awk -F. '{print $1}'`
LIBVIRT_QPID_MINOR_VERSION=`echo $VERSION | awk -F. '{print $2}'`
LIBVIRT_QPID_MICRO_VERSION=`echo $VERSION | awk -F. '{print $3}'`

AC_SUBST([LIBVIRT_QPID_MAJOR_VERSION])
AC_SUBST([LIBVIRT_QPID_MINOR_VERSION])
AC_SUBST([LIBVIRT_QPID_MICRO_VERSION])

dnl Required minimum versions of all libs we depend on
LIBVIRT_REQUIRED="4.4.0"

PKG_CHECK_MODULES(XML, libxml-2.0)

AC_OUTPUT(Makefile src/Makefile doc/Makefile)

