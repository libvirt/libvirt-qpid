
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <string.h>

#include "NodeWrap.h"
#include "PoolWrap.h"
#include "VolumeWrap.h"
#include "Error.h"

#include "ArgsPoolCreateVolumeXML.h"
#include "ArgsPoolGetXMLDesc.h"

namespace _qmf = qmf::com::redhat::libvirt;

PoolWrap::PoolWrap(ManagementAgent *_agent, NodeWrap *parent,
                   virStoragePoolPtr _pool_pointer, virConnectPtr _connect) :
                   agent(_agent), pool_ptr(_pool_pointer), conn(_connect)
{
    int ret;
    char pool_uuid_str[VIR_UUID_STRING_BUFLEN];
    const char *pool_name_str;
    char *parent_volume = NULL;

    pool = NULL;

    ret = virStoragePoolGetUUIDString(pool_ptr, pool_uuid_str);
    if (ret < 0) {
        REPORT_ERR(conn, "PoolWrap: Unable to get UUID\n");
        throw 1;
    }

    pool_name_str = virStoragePoolGetName(pool_ptr);
    if (pool_name_str == NULL) {
        REPORT_ERR(conn, "PoolWrap: error getting pool name\n");
        throw 1;
    }

    pool_sources_xml = virConnectFindStoragePoolSources(conn, "logical", NULL, 0);

    if (pool_sources_xml) {
        xmlDocPtr doc;
        xmlNodePtr cur;

        doc = xmlParseMemory(pool_sources_xml, strlen(pool_sources_xml));

        if (doc == NULL ) {
            goto done;
        }

        cur = xmlDocGetRootElement(doc);

        if (cur == NULL) {
            xmlFreeDoc(doc);
            goto done;
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
                    if (strcmp(pool_name_str, (char *) name) == 0) {
                        virStorageVolPtr vol;
                        vol = virStorageVolLookupByPath(conn, (char *) path);
                        if (vol != NULL) {
                            printf ("found storage volume associated with pool!\n");
                            parent_volume = virStorageVolGetPath(vol);
                            printf ("xml returned device name %s, path %s; volume path is %s\n", name, path, parent_volume);
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

done:

    pool_name_str = virStoragePoolGetName(pool_ptr);

    pool_name = pool_name_str;
    pool_uuid = pool_uuid_str;

    pool = new _qmf::Pool(agent, this, parent, pool_uuid, pool_name, parent_volume ? parent_volume : "");
    agent->addObject(pool);

    // Call refresh storage volumes in case anything changed in libvirt.
    // I don't think we're too concerned if it fails?
    virStoragePoolRefresh(pool_ptr, 0);

    // Set storage pool state to an inactive. Should the state be different, the
    // subsequent call to update will pick it up and fix it.
    storagePoolState = VIR_STORAGE_POOL_INACTIVE;

    // Call update() here so we set the state and see if there are any volumes
    // before returning the new object.
    update();
}

PoolWrap::~PoolWrap()
{
    // Destroy volumes..
    for (std::vector<VolumeWrap*>::iterator iter = volumes.begin(); iter != volumes.end();) {
        delete (*iter);
        iter = volumes.erase(iter);
    }

    if (pool) {
        pool->resourceDestroy();
    }
    virStoragePoolFree(pool_ptr);
}

char *
PoolWrap::getPoolSourcesXml()
{
    return pool_sources_xml;
}

void
PoolWrap::syncVolumes()
{
    int maxactive;
    int ret;
    int i;
    virStoragePoolInfo info;

    cout << "Syncing volumes.\n";

    ret = virStoragePoolGetInfo(pool_ptr, &info);
    if (ret < 0) {
        REPORT_ERR(conn, "PoolWrap: Unable to get info of storage pool");
        return;
    }

    // Only try to list volumes if the storage pool is active.
    if (info.state != VIR_STORAGE_POOL_INACTIVE) {

        maxactive = virStoragePoolNumOfVolumes(pool_ptr);
        if (maxactive < 0) {
            //vshError(ctl, FALSE, "%s", _("Failed to list active vols"));
            REPORT_ERR(conn, "error getting number of volumes in pool\n");
            return;
        }

        char **names;
        names = (char **) malloc(sizeof(char *) * maxactive);

        ret = virStoragePoolListVolumes(pool_ptr, names, maxactive);
        if (ret < 0) {
            REPORT_ERR(conn, "error getting list of volumes\n");
            return;
        }

        for (i = 0; i < ret; i++) {
            virStorageVolPtr vol_ptr;
            bool found = false;
            char *volume_name = names[i];

            for (std::vector<VolumeWrap*>::iterator iter = volumes.begin();
                 iter != volumes.end(); iter++) {
                if ((*iter)->volume_name == volume_name) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                vol_ptr = virStorageVolLookupByName(pool_ptr, volume_name);
                if (vol_ptr == NULL) {
                    REPORT_ERR(conn, "error looking up storage volume by name\n");
                    continue;
                }
                VolumeWrap *volume = NULL;
                try {
                    VolumeWrap *volume = new VolumeWrap(agent, this, vol_ptr, conn);
                    printf("Created new volume: %s, ptr is %p\n", volume_name, vol_ptr);
                    volumes.push_back(volume);
                } catch (int i) {
                    printf ("Error constructing volume\n");
                    REPORT_ERR(conn, "constructing volume.");
                    if (volume) {
                        delete volume;
                    }
                }
            }
        }

        for (i = 0; i < ret; i++) {
            free(names[i]);
        }
        free(names);
    }

    /* Go through our list of volumes and ensure that they still exist. */
    for (std::vector<VolumeWrap*>::iterator iter = volumes.begin(); iter != volumes.end();) {

        printf ("Verifying volume %s\n", (*iter)->volume_name.c_str());
        virStorageVolPtr ptr = virStorageVolLookupByName(pool_ptr, (*iter)->volume_name.c_str());
        if (ptr == NULL) {
            printf("Destroying volume %s\n", (*iter)->volume_name.c_str());
            delete(*iter);
            iter = volumes.erase(iter);
        } else {
            virStorageVolFree(ptr);
            iter++;
        }
    }

    /* And finally *phew*, call update() on all volumes. */
    for (std::vector<VolumeWrap*>::iterator iter = volumes.begin(); iter != volumes.end(); iter++) {
        (*iter)->update();
    }
}

void
PoolWrap::update()
{
    virStoragePoolInfo info;
    int ret;

    ret = virStoragePoolGetInfo(pool_ptr, &info);
    if (ret < 0) {
        REPORT_ERR(conn, "PoolWrap: Unable to get info of storage pool");
        return;
    }

    switch (info.state) {
        case VIR_STORAGE_POOL_INACTIVE:
            pool->set_state("inactive");
            break;
        case VIR_STORAGE_POOL_BUILDING:
            pool->set_state("building");
            break;
        case VIR_STORAGE_POOL_RUNNING:
            pool->set_state("running");
            break;
        case VIR_STORAGE_POOL_DEGRADED:
            pool->set_state("degraded");
            break;
    }

    pool->set_capacity(info.capacity);
    pool->set_allocation(info.allocation);
    pool->set_available(info.available);

    // Check if state has changed compared to stored state. If so, rescan
    // storage pool sources (eg. logical pools on a lun might now be visible)
    if (storagePoolState != info.state)
        pool_sources_xml = virConnectFindStoragePoolSources(conn, "logical", NULL, 0);

    storagePoolState = info.state;

    // Sync volumes after (potentially) rescanning for logical storage pool sources
    // so we pick up any new pools if the state of this pool changed.
    syncVolumes();
}

Manageable::status_t
PoolWrap::ManagementMethod(uint32_t methodId, Args& args, std::string &errstr)
{
    cout << "Method Received: " << methodId << endl;
    int ret;

    switch (methodId) {
        case _qmf::Pool::METHOD_GETXMLDESC:
        {
            _qmf::ArgsPoolGetXMLDesc *ioArgs = (_qmf::ArgsPoolGetXMLDesc *) &args;
            char *desc;

            desc = virStoragePoolGetXMLDesc(pool_ptr, 0);
            if (desc) {
                ioArgs->o_description = desc;
            } else {
                errstr = FORMAT_ERR(conn, "Error getting XML description of storage pool (virStoragePoolGetXMLDesc).", &ret);
                return STATUS_USER + ret;
            }
            return STATUS_OK;
        }

        case _qmf::Pool::METHOD_CREATE:
        {
            ret = virStoragePoolCreate(pool_ptr, 0);
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error creating new storage pool (virStoragePoolCreate).", &ret);
                return STATUS_USER + ret;
            }
            update();
            return STATUS_OK;
        }

        case _qmf::Pool::METHOD_BUILD:
        {
            ret = virStoragePoolBuild(pool_ptr, 0);
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error building storage pool (virStoragePoolBuild).", &ret);
                return STATUS_USER + ret;
            }
            update();
            return STATUS_OK;
        }

        case _qmf::Pool::METHOD_DESTROY:
        {
            ret = virStoragePoolDestroy(pool_ptr);
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error destroying storage pool (virStoragePoolDestroy).", &ret);
                return STATUS_USER + ret;
            }
            update();
            return STATUS_OK;
        }

        case _qmf::Pool::METHOD_DELETE:
        {
            ret = virStoragePoolDelete(pool_ptr, 0);
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error deleting storage pool (virStoragePoolDelete).", &ret);
                return STATUS_USER + ret;
            }
            update();
            return STATUS_OK;
        }

        case _qmf::Pool::METHOD_UNDEFINE:
        {
            ret = virStoragePoolUndefine(pool_ptr);
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error undefining storage pool (virStoragePoolUndefine).", &ret);
                return STATUS_USER + ret;
            }
            update();
            return STATUS_OK;
        }

        case _qmf::Pool::METHOD_CREATEVOLUMEXML:
        {
            _qmf::ArgsPoolCreateVolumeXML *io_args = (_qmf::ArgsPoolCreateVolumeXML *) &args;
            virStorageVolPtr volume_ptr;

            volume_ptr = virStorageVolCreateXML(pool_ptr, io_args->i_xmlDesc.c_str(), 0);
            if (volume_ptr == NULL) {
                errstr = FORMAT_ERR(conn, "Error creating new storage volume from XML description (virStorageVolCreateXML).", &ret);
                return STATUS_USER + ret;
            }

            VolumeWrap *volume;
            try {
                volume = new VolumeWrap(agent, this, volume_ptr, conn);
                volumes.push_back(volume);
                io_args->o_volume = volume->GetManagementObject()->getObjectId();
            } catch (int i) {
                delete volume;
                errstr = FORMAT_ERR(conn, "Error constructing pool object in virStorageVolCreateXML.", &ret);
                return STATUS_USER + i;
            }

            update();
            return STATUS_OK;
        }
        
        case _qmf::Pool::METHOD_REFRESH:
        {
            ret = virStoragePoolRefresh(pool_ptr, 0);
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error refreshing storage pool (virStoragePoolRefresh).", &ret);
                return STATUS_USER + ret;
            }
            update();
            return STATUS_OK;
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}


