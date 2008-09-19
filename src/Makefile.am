## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src/qmf/com/redhat/libvirt

bin_PROGRAMS = libvirt-qpid

generated_file_list = \
	qmf/com/redhat/libvirt/ArgsDomainMigrate.h \
	qmf/com/redhat/libvirt/ArgsDomainRestore.h \
	qmf/com/redhat/libvirt/ArgsDomainSave.h \
	qmf/com/redhat/libvirt/ArgsDomainXml_desc.h \
	qmf/com/redhat/libvirt/ArgsNodeDomain_define_xml.h \
	qmf/com/redhat/libvirt/ArgsNodeStorage_pool_create_xml.h \
	qmf/com/redhat/libvirt/ArgsNodeStorage_pool_define_xml.h \
	qmf/com/redhat/libvirt/ArgsPoolCreate_volume_xml.h \
	qmf/com/redhat/libvirt/ArgsPoolXml_desc.h \
	qmf/com/redhat/libvirt/ArgsVolumeXml_desc.h \
	qmf/com/redhat/libvirt/Domain.cpp \
	qmf/com/redhat/libvirt/Domain.h \
	qmf/com/redhat/libvirt/Node.cpp \
	qmf/com/redhat/libvirt/Node.h \
	qmf/com/redhat/libvirt/NodeInfo.cpp \
	qmf/com/redhat/libvirt/NodeInfo.h \
	qmf/com/redhat/libvirt/Package.cpp \
	qmf/com/redhat/libvirt/Package.h \
	qmf/com/redhat/libvirt/Pool.cpp \
	qmf/com/redhat/libvirt/Pool.h \
	qmf/com/redhat/libvirt/Volume.cpp \
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

libvirt_qpid_LDADD = -lqpidclient -lqpidcommon -lqmfagent -lvirt

dist_pkgdata_DATA = libvirt-schema.xml

#CLEANFILES = *.gcov .libs/*.gcda .libs/*.gcno *.gcno *.gcda


