
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

    ret = virStoragePoolGetUUIDString(pool_ptr, pool_uuid_str);
    if (ret < 0) {
        REPORT_ERR(conn, "PoolWrap: Unable to get UUID\n");
        return;
    }

    pool_name_str = virStoragePoolGetName(pool_ptr);
    if (pool_name_str == NULL) {
        REPORT_ERR(conn, "PoolWrap: error getting pool name\n");
    }

    pool_name = pool_name_str;
    pool_uuid = pool_uuid_str;

    pool = new _qmf::Pool(agent, this, parent, pool_uuid, pool_name);
    agent->addObject(pool);
}

PoolWrap::~PoolWrap()
{
    pool->resourceDestroy();
    virStoragePoolFree(pool_ptr);
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

                VolumeWrap *volume = new VolumeWrap(agent, this, vol_ptr, conn);
                printf("Created new volume: %s, ptr is %p\n", volume_name, vol_ptr);
                volumes.push_back(volume);
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
            delete (*iter);
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

            desc = virStoragePoolGetXMLDesc(pool_ptr, VIR_DOMAIN_XML_SECURE | VIR_DOMAIN_XML_INACTIVE);
            if (desc) {
                ioArgs->o_description = desc;
            } else {
                errstr = FORMAT_ERR(conn, "Error getting XML description of storage pool (virStoragePoolGetXMLDesc).");
                return STATUS_USER;
            }
            return STATUS_OK;
        }

        case _qmf::Pool::METHOD_CREATE:
        {
            ret = virStoragePoolCreate(pool_ptr, 0);
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error creating new storage pool (virStoragePoolCreate).");
                return STATUS_USER;
            }
            return STATUS_OK;
        }

        case _qmf::Pool::METHOD_DESTROY:
        {
            ret = virStoragePoolDestroy(pool_ptr);
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error destroying storage pool (virStoragePoolDestroy).");
                return STATUS_USER;
            }
            return STATUS_OK;
        }

        case _qmf::Pool::METHOD_DELETE:
        {
            ret = virStoragePoolDelete(pool_ptr, 0);
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error deleting storage pool (virStoragePoolDelete).");
                return STATUS_USER;
            }
            return STATUS_OK;
        }

        case _qmf::Pool::METHOD_CREATEVOLUMEXML:
        {
            qpid::framing::Buffer buffer;
            _qmf::ArgsPoolCreateVolumeXML *io_args = (_qmf::ArgsPoolCreateVolumeXML *) &args;
            virStorageVolPtr volume_ptr;

            volume_ptr = virStorageVolCreateXML(pool_ptr, io_args->i_xmlDesc.c_str(), 0);
            if (volume_ptr == NULL) {
                errstr = FORMAT_ERR(conn, "Error creating new storage volume from XML description (virStorageVolCreateXML).");
                return STATUS_USER;
            }

            VolumeWrap *volume = new VolumeWrap(agent, this, volume_ptr, conn);
            io_args->o_volume = volume->GetManagementObject()->getObjectId();

            return STATUS_OK;
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}


