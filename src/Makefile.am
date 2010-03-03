## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src/qmf/com/redhat/libvirt $(XML_CFLAGS)

sbin_PROGRAMS = libvirt-qpid

generated_file_list = \
	qmf/com/redhat/libvirt/Domain.cpp\
	qmf/com/redhat/libvirt/Node.cpp\
	qmf/com/redhat/libvirt/Package.cpp\
	qmf/com/redhat/libvirt/Pool.cpp\
	qmf/com/redhat/libvirt/Volume.cpp\
	qmf/com/redhat/libvirt/ArgsDomainGetXMLDesc.h\
	qmf/com/redhat/libvirt/ArgsDomainMigrate.h\
	qmf/com/redhat/libvirt/ArgsDomainRestore.h\
	qmf/com/redhat/libvirt/ArgsDomainSave.h\
	qmf/com/redhat/libvirt/ArgsNodeDomainDefineXML.h\
	qmf/com/redhat/libvirt/ArgsNodeStoragePoolCreateXML.h\
	qmf/com/redhat/libvirt/ArgsNodeStoragePoolDefineXML.h\
	qmf/com/redhat/libvirt/ArgsPoolCreateVolumeXML.h\
	qmf/com/redhat/libvirt/ArgsPoolGetXMLDesc.h\
	qmf/com/redhat/libvirt/ArgsVolumeGetXMLDesc.h\
	qmf/com/redhat/libvirt/Domain.h\
	qmf/com/redhat/libvirt/Node.h\
	qmf/com/redhat/libvirt/Package.h\
	qmf/com/redhat/libvirt/Pool.h\
	qmf/com/redhat/libvirt/Volume.h

nodist_libvirt_qpid_SOURCES = $(generated_file_list)

libvirt_qpid_SOURCES = \
	DomainWrap.cpp \
	Error.cpp \
	NodeWrap.cpp \
	PoolWrap.cpp \
	VolumeWrap.cpp \
	DomainWrap.h \
	Error.h \
	NodeWrap.h \
	PoolWrap.h \
	VolumeWrap.h 

$(generated_file_list): libvirt-schema.xml
	qmf-gen -o ./qmf ./libvirt-schema.xml

BUILT_SOURCES = $(generated_file_list)
CLEANFILES = $(generated_file_list)

libvirt_qpid_LDADD = -lqmf -lqpidcommon -lqpidclient -lvirt $(XML_LIBS)

dist_pkgdata_DATA = libvirt-schema.xml

#CLEANFILES = *.gcov .libs/*.gcda .libs/*.gcno *.gcno *.gcda


