#include <qpid/management/Manageable.h>
#include <qpid/management/ManagementObject.h>
#include <qpid/agent/ManagementAgent.h>
#include <qpid/sys/Mutex.h>

#include "Package.h"
#include "Volume.h"

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
class PoolWrap;

class VolumeWrap : public Manageable
{
    ManagementAgent *agent;
    qmf::com::redhat::libvirt::Volume *volume;

    std::string volume_key;
    std::string volume_path;

    std::string lvm_name;
    bool has_lvm_child;

    virConnectPtr conn;
    virStorageVolPtr volume_ptr;

    PoolWrap *wrap_parent;

    void checkForLVMPool();

public:

    std::string volume_name;

    VolumeWrap(ManagementAgent *agent, PoolWrap *parent, virStorageVolPtr pool_ptr, virConnectPtr connect);
    ~VolumeWrap();

    ManagementObject* GetManagementObject(void) const
    {
        return volume;
    }

    void update();

    status_t ManagementMethod (uint32_t methodId, Args& args, std::string &errstr);
};



