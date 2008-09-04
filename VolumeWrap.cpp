#include <string.h>
#include <sys/select.h>
#include <errno.h>

#include "NodeWrap.h"
#include "PoolWrap.h"
#include "VolumeWrap.h"


VolumeWrap::VolumeWrap(ManagementAgent *agent, PoolWrap *parent, 
                       virStorageVolPtr volume_pointer, virConnectPtr connect)
{
    int ret;
    const char *volume_key;
    char *volume_path;
    const char *volume_name;

    volume_ptr = volume_pointer;
    conn = connect;

    volume_key = virStorageVolGetKey(volume_ptr);
    if (volume_key == NULL) {
        printf ("Error getting storage volume key\n");
        return;
    }

    volume_path = virStorageVolGetPath(volume_ptr);
    if (volume_path == NULL) {
        printf ("Error getting volume path\n");
        return;
    }

    volume_name = virStorageVolGetName(volume_ptr);
    if (volume_name == NULL) {
        printf ("Error getting volume name\n");
        return;
    }

    volume = new Volume(agent, this, parent, volume_key, volume_path, volume_name);
    agent->addObject(volume);
}

/*
update()
{
    virStorageVolumeInfo info;

    ret = virStorageVolumeGetInfo(volume_ptr, &info);
    if (ret < 0) {
        printf("VolumeWrap: Unable to get info of storage volume\n");
        return;
    }
}
*/

VolumeWrap::~VolumeWrap()
{
    volume->resourceDestroy();
    virStorageVolFree(volume_ptr);
}

Manageable::status_t
VolumeWrap::ManagementMethod(uint32_t methodId, Args& args)
{
    Mutex::ScopedLock _lock(vectorLock);
    cout << "Method Received: " << methodId << endl;
    int ret;

    switch (methodId) {
    }

    return STATUS_NOT_IMPLEMENTED;
}


