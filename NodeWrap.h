
#include <qpid/management/Manageable.h>
#include <qpid/management/ManagementObject.h>
#include <qpid/agent/ManagementAgent.h>
#include <qpid/sys/Mutex.h>

#include "PackageLibvirt.h"
#include "Node.h"
#include "Domain.h"
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


// Forward decl of DomainWrap to get around cyclic reference.
class DomainWrap;
class PoolWrap;

class NodeWrap : public Manageable
{
    string name;
    ManagementAgent *agent;
    Node *mgmtObject;
    Mutex vectorLock;
    std::vector<DomainWrap*> domains;
    std::vector<PoolWrap*> pools;

    virConnectPtr conn;

public:

    NodeWrap(ManagementAgent* agent, string _name);
    ~NodeWrap() { mgmtObject->resourceDestroy(); }

    ManagementObject* GetManagementObject(void) const
    { return mgmtObject; }

    void doLoop();
    void syncDomains();
    void checkPool(char *pool_name);
    void syncPools();

    status_t ManagementMethod (uint32_t methodId, Args& args);
};


