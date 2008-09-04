
#include "NodeWrap.h"
#include "PoolWrap.h"
#include "VolumeWrap.h"

#include "ArgsPoolCreate_volume_xml.h"
#include "ArgsPoolXml_desc.h"
#include "ArgsVolumeXml_desc.h"

PoolWrap::PoolWrap(ManagementAgent *agent, NodeWrap *parent,
                   virStoragePoolPtr pool_pointer, virConnectPtr connect)
{
    int ret;
    char pool_uuid_str[VIR_UUID_STRING_BUFLEN];
    const char *pool_name_str;

    ret = virStoragePoolGetUUIDString(pool_ptr, pool_uuid_str);
    if (ret < 0) {
        printf("PoolWrap: Unable to get UUID\n");
        return;
    }

    pool_name_str = virStoragePoolGetName(pool_ptr);
    if (pool_name_str == NULL) {
        printf ("PoolWrap: error getting pool name\n");
    }

    pool_name = pool_name_str;
    pool_uuid = pool_uuid_str;

    pool = new Pool(agent, this, parent, pool_uuid, pool_name);
    agent->addObject(pool);
    conn = connect;
    pool_ptr = pool_pointer;
}

/*
update()
{
    virStoragePoolInfo info;

    ret = virStoragePoolGetInfo(pool_ptr, &info);
    if (ret < 0) {
        printf("PoolWrap: Unable to get info of storage pool\n");
        return;
    }
}
*/

PoolWrap::~PoolWrap()
{
    pool->resourceDestroy();
    virStoragePoolFree(pool_ptr);
}

Manageable::status_t
PoolWrap::ManagementMethod(uint32_t methodId, Args& args)
{
    Mutex::ScopedLock _lock(vectorLock);
    cout << "Method Received: " << methodId << endl;
    int ret;

    switch (methodId) {
        case Pool::METHOD_XML_DESC:
        {
            ArgsPoolXml_desc *ioArgs = (ArgsPoolXml_desc *) &args;
            char *desc;

            desc = virStoragePoolGetXMLDesc(pool_ptr, VIR_DOMAIN_XML_SECURE | VIR_DOMAIN_XML_INACTIVE);
            if (desc) {
                ioArgs->o_description = desc;
            } else {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;
        }

        case Pool::METHOD_CREATE:
        {
            ret = virStoragePoolCreate(pool_ptr, 0);
            if (ret < 0) {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;
        }

        case Pool::METHOD_DESTROY:
        {
            ret = virStoragePoolDestroy(pool_ptr);
            if (ret < 0) {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;
        }

        case Pool::METHOD_DELETE:
        {
            ret = virStoragePoolDelete(pool_ptr, 0);
            if (ret < 0) {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;
        }

        case Pool::METHOD_CREATE_VOLUME_XML:
        {
            qpid::framing::Buffer buffer;
            ArgsPoolCreate_volume_xml *io_args = (ArgsPoolCreate_volume_xml *) &args;
            virStorageVolPtr volume_ptr;

            volume_ptr = virStorageVolCreateXML(pool_ptr, io_args->i_xml_desc.c_str(), 0);

            VolumeWrap *volume = new VolumeWrap(agent, this, volume_ptr, conn);

            volume->GetManagementObject()->getObjectId().encode(buffer);
            // FIXME: 256??
            buffer.getRawData(io_args->o_volume, 256);
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}


