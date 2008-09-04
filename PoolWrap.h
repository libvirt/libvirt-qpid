#include <qpid/management/Manageable.h>
#include <qpid/management/ManagementObject.h>
#include <qpid/agent/ManagementAgent.h>
#include <qpid/sys/Mutex.h>

#include "PackageLibvirt.h"
#include "Pool.h"

#include <unistd.h>
#include <cstdlib>
#include <iostream>

#include <sstream>

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>

using namespace qpid::management;
using namespace qpid::sys;
using namespace std;
using qpid::management::ManagementObject;
using qpid::management::Manageable;
using qpid::management::Args;
using qpid::sys::Mutex;


class PoolWrap : public Manageable
{
    ManagementAgent *agent;
    Pool *pool;
    Mutex vectorLock;

    virConnectPtr conn;
    virStoragePoolPtr pool_ptr;

public:

    PoolWrap(ManagementAgent *agent, NodeWrap *parent, virStoragePoolPtr pool_ptr, virConnectPtr connect);
    ~PoolWrap();

    ManagementObject* GetManagementObject(void) const
    {
        return pool;
    }

    status_t ManagementMethod (uint32_t methodId, Args& args);
};



