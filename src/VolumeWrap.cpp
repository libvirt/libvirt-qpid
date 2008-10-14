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

    volume_key_s = virStorageVolGetKey(volume_ptr);
    if (volume_key_s == NULL) {
        REPORT_ERR(conn, "Error getting storage volume key\n");
        return;
    }
    volume_key = volume_key_s;

    volume_path_s = virStorageVolGetPath(volume_ptr);
    if (volume_path_s == NULL) {
        REPORT_ERR(conn, "Error getting volume path\n");
        return;
    }
    volume_path = volume_path_s;

    volume_name_s = virStorageVolGetName(volume_ptr);
    if (volume_name_s == NULL) {
        REPORT_ERR(conn, "Error getting volume name\n");
        return;
    }
    volume_name = volume_name_s;

    volume = new _qmf::Volume(agent, this, parent, volume_key, volume_path, volume_name);
    printf("adding volume to agent - volume %p\n", volume);
    agent->addObject(volume);
    printf("done\n");
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
    volume->resourceDestroy();
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

            desc = virStorageVolGetXMLDesc(volume_ptr, VIR_DOMAIN_XML_SECURE | VIR_DOMAIN_XML_INACTIVE);
            if (desc) {
                io_args->o_description = desc;
            } else {
                errstr = FORMAT_ERR(conn, "Error getting xml description for volume (virStorageVolGetXMLDesc).", &ret);
                return STATUS_USER + ret;
            }
            return STATUS_OK;
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}


