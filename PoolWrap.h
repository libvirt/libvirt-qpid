#include <qpid/management/Manageable.h>
#include <qpid/management/ManagementObject.h>
#include <qpid/agent/ManagementAgent.h>
#include <qpid/sys/Mutex.h>

#include "qmf/com/redhat/libvirt/Package.h"
#include "qmf/com/redhat/libvirt/Pool.h"

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

// Forward decl.
class VolumeWrap;

class PoolWrap : public Manageable
{
    ManagementAgent *agent;
    qmf::com::redhat::libvirt::Pool *pool;

    virConnectPtr conn;
    virStoragePoolPtr pool_ptr;

    std::vector<VolumeWrap*> volumes;

public:

    PoolWrap(ManagementAgent *agent, NodeWrap *parent, virStoragePoolPtr pool_ptr, virConnectPtr connect);
    ~PoolWrap();

    std::string pool_name;
    std::string pool_uuid;

    ManagementObject* GetManagementObject(void) const
    {
        return pool;
    }

    void update();
    void syncVolumes();

    status_t ManagementMethod (uint32_t methodId, Args& args, std::string &errstr);
};



