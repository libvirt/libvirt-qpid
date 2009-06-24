#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <string.h>
#include <sys/select.h>
#include <errno.h>

#include "NodeWrap.h"
#include "PoolWrap.h"
#include "VolumeWrap.h"
#include "Error.h"

#include "ArgsVolumeGetXMLDesc.h"

namespace _qmf = qmf::com::redhat::libvirt;

VolumeWrap::VolumeWrap(ManagementAgent *agent, PoolWrap *parent,
                       virStorageVolPtr _volume_ptr, virConnectPtr _conn) :
                       conn(_conn), volume_ptr(_volume_ptr)
{
    const char *volume_key_s;
    char *volume_path_s;
    const char *volume_name_s;

    volume = NULL;

    wrap_parent = parent;

    volume_key_s = virStorageVolGetKey(volume_ptr);
    if (volume_key_s == NULL) {
        REPORT_ERR(conn, "Error getting storage volume key\n");
        throw 1;
    }
    volume_key = volume_key_s;

    volume_path_s = virStorageVolGetPath(volume_ptr);
    if (volume_path_s == NULL) {
        REPORT_ERR(conn, "Error getting volume path\n");
        throw 1;
    }
    volume_path = volume_path_s;

    volume_name_s = virStorageVolGetName(volume_ptr);
    if (volume_name_s == NULL) {
        REPORT_ERR(conn, "Error getting volume name\n");
        throw 1;
    }
    volume_name = volume_name_s;

    lvm_name = "";
    has_lvm_child = false;

    checkForLVMPool();

    volume = new _qmf::Volume(agent, this, parent, volume_key, volume_path, volume_name, lvm_name);
    printf("adding volume to agent - volume %p\n", volume);
    agent->addObject(volume);

    printf("done\n");
}

void
VolumeWrap::checkForLVMPool()
{
    char *real_path = NULL;
    char *pool_sources_xml;

    pool_sources_xml = wrap_parent->getPoolSourcesXml();

    if (pool_sources_xml) {
        xmlDocPtr doc;
        xmlNodePtr cur;

        doc = xmlParseMemory(pool_sources_xml, strlen(pool_sources_xml));

        if (doc == NULL ) {
            return;
        }

        cur = xmlDocGetRootElement(doc);

        if (cur == NULL) {
            xmlFreeDoc(doc);
            return;
        }

        xmlChar *path = NULL;
        xmlChar *name = NULL;

        cur = cur->xmlChildrenNode;
        while (cur != NULL) {
            xmlNodePtr source;
            if ((!xmlStrcmp(cur->name, (const xmlChar *) "source"))) {
                source = cur->xmlChildrenNode;
                while (source != NULL) {
                    if ((!xmlStrcmp(source->name, (const xmlChar *) "device"))) {
                        path = xmlGetProp(source, (const xmlChar *) "path");
                    }

                    if ((!xmlStrcmp(source->name, (const xmlChar *) "name"))) {
                        name = xmlNodeListGetString(doc, source->xmlChildrenNode, 1);
                    }

                source = source->next;
                }
                if (name && path) {
                    virStorageVolPtr vol;

                    printf ("xml returned device name %s, path %s; volume path is %s\n", name, path, volume_path.c_str());
                    vol = virStorageVolLookupByPath(conn, (char *) path);
                    if (vol != NULL) {
                        real_path = virStorageVolGetPath(vol);
                        if (real_path && strcmp(real_path, volume_path.c_str()) == 0) {
                            printf ("found matching storage volume associated with pool!\n");
                            lvm_name.assign((char *) name);
                            has_lvm_child = true;
                        }
                    }
                    xmlFree(path);
                    xmlFree(name);
                    path = NULL;
                    name = NULL;
                }
            }
            cur = cur->next;
        }
        xmlFreeDoc(doc);
    }
}

void
VolumeWrap::update()
{
    virStorageVolInfo info;
    int ret;

    printf("Updating volume info\n");

    ret = virStorageVolGetInfo(volume_ptr, &info);
    if (ret < 0) {
        REPORT_ERR(conn, "VolumeWrap: Unable to get info of storage volume info\n");
        return;
    }
    volume->set_capacity(info.capacity);
    volume->set_allocation(info.allocation);
}

VolumeWrap::~VolumeWrap()
{
    if (volume) {
        volume->resourceDestroy();
    }
    virStorageVolFree(volume_ptr);
}

Manageable::status_t
VolumeWrap::ManagementMethod(uint32_t methodId, Args& args, std::string &errstr)
{
    cout << "Method Received: " << methodId << endl;
    int ret;

    switch (methodId) {
    case _qmf::Volume::METHOD_GETXMLDESC:
        {
            _qmf::ArgsVolumeGetXMLDesc *io_args = (_qmf::ArgsVolumeGetXMLDesc *) &args;
            char *desc;

            desc = virStorageVolGetXMLDesc(volume_ptr, 0);
            if (desc) {
                io_args->o_description = desc;
            } else {
                errstr = FORMAT_ERR(conn, "Error getting xml description for volume (virStorageVolGetXMLDesc).", &ret);
                return STATUS_USER + ret;
            }
            return STATUS_OK;
        }
    case _qmf::Volume::METHOD_DELETE:
        {
            ret = virStorageVolDelete(volume_ptr, 0);
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error deleting storage volume (virStorageVolDelete).", &ret);
                return STATUS_USER + ret;
            }
            update();
            return STATUS_OK;
        }
    }


    return STATUS_NOT_IMPLEMENTED;
}


