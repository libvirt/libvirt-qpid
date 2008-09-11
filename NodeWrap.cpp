
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#include "NodeWrap.h"
#include "DomainWrap.h"
#include "PoolWrap.h"
#include "Error.h"

#include "ArgsNodeDomain_define_xml.h"
#include "ArgsNodeStorage_pool_create_xml.h"
#include "ArgsNodeStorage_pool_define_xml.h"


NodeWrap::NodeWrap(ManagementAgent* _agent, string _name) : name(_name), agent(_agent)
{
    char *hostname;
    char libvirt_version[256] = "Unknown";
    char api_version[256] = "Unknown";
    char hv_version[256] = "Unknown";
    char *uri;
    const char *hv_type;
    unsigned long api_v;
    unsigned long libvirt_v;
    unsigned long hv_v;
    int ret;
    unsigned int major;
    unsigned int minor;
    unsigned int rel;

    conn = virConnectOpen(NULL);
    if (!conn) {
        REPORT_ERR(conn, "virConnectOpen");
        exit(1);
    }

    hostname = virConnectGetHostname(conn);
    if (hostname == NULL) {
        REPORT_ERR(conn, "virConnectGetHostname");
        exit(1);
    }

    hv_type = virConnectGetType(conn);
    if (hv_type == NULL) {
        REPORT_ERR(conn, "virConnectGetType");
        exit(1);
    }

    uri = virConnectGetURI(conn);
    if (uri == NULL) {
        REPORT_ERR(conn, "virConnectGetURI");
        exit(1);
    }

    ret = virGetVersion(&libvirt_v, hv_type, &api_v);
    if (ret < 0) {
        REPORT_ERR(conn, "virGetVersion");
    } else {
        major = libvirt_v / 1000000;
        libvirt_v %= 1000000;
        minor = libvirt_v / 1000;
        rel = libvirt_v % 1000;
        snprintf(libvirt_version, sizeof(libvirt_version), "%d.%d.%d", major, minor, rel);

        major = api_v / 1000000;
        api_v %= 1000000;
        minor = api_v / 1000;
        rel = api_v % 1000;
        snprintf(api_version, sizeof(api_version), "%d.%d.%d", major, minor, rel);
    }

    ret = virConnectGetVersion(conn, &hv_v);
    if (ret < 0) {
        REPORT_ERR(conn, "virConnectGetVersion");
    } else {
        major = hv_v / 1000000;
        hv_v %= 1000000;
        minor = hv_v / 1000;
        rel = hv_v % 1000;
        snprintf(hv_version, sizeof(hv_version), "%d.%d.%d", major, minor, rel);
    }

    mgmtObject = new Node(agent, this, hostname, uri, libvirt_version, api_version, hv_version, hv_type);
    agent->addObject(mgmtObject);
}

void NodeWrap::syncDomains()
{

    /* Sync up with domains that are defined but not active. */
    int maxname = virConnectNumOfDefinedDomains(conn);
    if (maxname < 0) {
        REPORT_ERR(conn, "virConnectNumOfDefinedDomains");
        return;
    } else {
        char *names[maxname];

        if ((maxname = virConnectListDefinedDomains(conn, names, maxname)) < 0) {
            REPORT_ERR(conn, "virConnectListDefinedDomains");
            return;
        }


        for (int i = 0; i < maxname; i++) {
            virDomainPtr domain_ptr;

            bool found = false;
            for (std::vector<DomainWrap*>::iterator iter = domains.begin();
                    iter != domains.end(); iter++) {
                if ((*iter)->domain_name == names[i]) {
                    found = true;
                    break;
                }
            }

            if (found) {
                continue;
            }

            domain_ptr = virDomainLookupByName(conn, names[i]);
            if (!domain_ptr) {
                REPORT_ERR(conn, "virDomainLookupByName");
            } else {
                DomainWrap *domain = new DomainWrap(agent, this, domain_ptr, conn);
                printf("Created new domain: %s, ptr is %p\n", names[i], domain_ptr);
                domains.push_back(domain);
            }
        }
    }

    /* Go through all the active domains */
    int maxids = virConnectNumOfDomains(conn);
    if (maxids < 0) {
        REPORT_ERR(conn, "virConnectNumOfDomains");
        return;
    } else {
        int ids[maxids];

        if ((maxids = virConnectListDomains(conn, ids, maxids)) < 0) {
            printf("Error getting list of defined domains\n");
            return;
        }

        for (int i = 0; i < maxids; i++) {
            virDomainPtr domain_ptr;
            char dom_uuid[VIR_UUID_BUFLEN];

            domain_ptr = virDomainLookupByID(conn, ids[i]);
            if (!domain_ptr) {
                REPORT_ERR(conn, "virDomainLookupByID");
                continue;
            }

            if (virDomainGetUUIDString(domain_ptr, dom_uuid) < 0) {
                REPORT_ERR(conn, "virDomainGetUUIDString");
                continue;
            }

            bool found = false;
            for (std::vector<DomainWrap*>::iterator iter = domains.begin();
                    iter != domains.end(); iter++) {
                if (strcmp((*iter)->domain_uuid.c_str(), dom_uuid) == 0) {
                    found = true;
                    break;
                }
            }

            if (found) {
                virDomainFree(domain_ptr);
                continue;
            }

            DomainWrap *domain = new DomainWrap(agent, this, domain_ptr, conn);
            printf("Created new domain: %d, ptr is %p\n", ids[i], domain_ptr);
            domains.push_back(domain);
        }
    }

    /* Go through our list of domains and ensure that they still exist. */
    for (std::vector<DomainWrap*>::iterator iter = domains.begin(); iter != domains.end();) {

        printf ("verifying domain %s\n", (*iter)->domain_name.c_str());
        virDomainPtr ptr = virDomainLookupByUUIDString(conn, (*iter)->domain_uuid.c_str());
        if (ptr == NULL) {
            REPORT_ERR(conn, "virDomainLookupByUUIDString");
            delete (*iter);
            iter = domains.erase(iter);
        } else {
            virDomainFree(ptr);
            iter++;
        }
    }
}

void NodeWrap::checkPool(char *pool_name)
{
    virStoragePoolPtr pool_ptr;

    bool found = false;
    for (std::vector<PoolWrap*>::iterator iter = pools.begin();
            iter != pools.end(); iter++) {
        if ((*iter)->pool_name == pool_name) {
            found = true;
            break;
        }
    }

    if (found) {
        return;
    }

    pool_ptr = virStoragePoolLookupByName(conn, pool_name);
    if (!pool_ptr) {
        REPORT_ERR(conn, "virStoragePoolLookupByName");
    } else {
        PoolWrap *pool = new PoolWrap(agent, this, pool_ptr, conn);
        printf("Created new pool: %s, ptr is %p\n", pool_name, pool_ptr);
        pools.push_back(pool);
    }
}

void NodeWrap::syncPools()
{
    int maxname;
    int maxinactive;

    maxname = virConnectNumOfStoragePools(conn);
    if (maxname < 0) {
        REPORT_ERR(conn, "virConnectNumOfStroagePools");
        return;
    } else {
        char *names[maxname];

        if ((maxname = virConnectListStoragePools(conn, names, maxname)) < 0) {
            REPORT_ERR(conn, "virConnectListStoragePools");
            return;
        }

        for (int i = 0; i < maxname; i++) {
            checkPool(names[i]);
        }
    }

    maxname = virConnectNumOfDefinedStoragePools(conn);
    if (maxname < 0) {
        REPORT_ERR(conn, "virConnectNumOfDefinedStoragePools");
        return;
    } else {
        char *names[maxname];

        if ((maxname = virConnectListDefinedStoragePools(conn, names, maxname)) < 0) {
            REPORT_ERR(conn, "virConnectListDefinedStoragePools");
            return;
        }

        for (int i = 0; i < maxname; i++) {
            checkPool(names[i]);
        }
    }

    /* Go through our list of pools and ensure that they still exist. */
    for (std::vector<PoolWrap*>::iterator iter = pools.begin(); iter != pools.end();) {

        printf ("Verifying pool %s\n", (*iter)->pool_name.c_str());
        virStoragePoolPtr ptr = virStoragePoolLookupByUUIDString(conn, (*iter)->pool_uuid.c_str());
        if (ptr == NULL) {
            printf("Destroying pool %s\n", (*iter)->pool_name.c_str());
            delete (*iter);
            iter = pools.erase(iter);
        } else {
            virStoragePoolFree(ptr);
            iter++;
        }
    }
}


void NodeWrap::doLoop()
{
    fd_set fds;
    struct timeval tv;
    int retval;

    /* Go through all domains and call update() for each, letting them update
     * information and statistics. */
    while (1) {
        Mutex::ScopedLock _lock(vectorLock);
        int read_fd = agent->getSignalFd();
        printf ("read ifd is %d\n", read_fd);

        // FIXME: I think we should check the libvirt connection pointer here and if it's invalid
        // we should go about trying to reconnect instead of all this other stuff. 

        /* Poll agent fd.  If any methods are being made this FD will be ready for reading.  */
        FD_ZERO(&fds);
        FD_SET(read_fd, &fds);

        /* Wait up to five seconds. */
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        retval = select(read_fd + 1, &fds, NULL, NULL, &tv);
        if (retval < 0) {
            fprintf (stderr, "Error in select loop: %s\n", strerror(errno));
            continue;
        }

        if (retval > 0) {
            /* This implements any pending methods. */
            agent->pollCallbacks();
        }

        // FIXME: This could be a little smarter.. as it is it fires everytime a
        // method is called.  This may be a good thing or it may need to be setup
        // so it only runs every 5 seconds or such.
        syncDomains();
        syncPools();

        for (std::vector<DomainWrap*>::iterator iter = domains.begin();
                iter != domains.end(); iter++) {
            (*iter)->update();
        }
        for (std::vector<PoolWrap*>::iterator iter = pools.begin();
                iter != pools.end(); iter++) {
            (*iter)->update();
        }
    }
}

Manageable::status_t
NodeWrap::ManagementMethod(uint32_t methodId, Args& args, std::string &errstr)
{
    virDomainPtr domain_ptr;
    Mutex::ScopedLock _lock(vectorLock);
    cout << "Method Received: " << methodId << endl;

    switch (methodId) {
        case Node::METHOD_DOMAIN_DEFINE_XML:
        {
            ArgsNodeDomain_define_xml *io_args = (ArgsNodeDomain_define_xml *) &args;
            domain_ptr = virDomainDefineXML(conn, io_args->i_xml_desc.c_str());
            if (!domain_ptr) {
                //REPORT_ERR(conn, "Error creating domain using xml description (virDomainDefineXML).");
                errstr = FORMAT_ERR(conn, "Error creating domain using xml description (virDomainDefineXML).");
                return STATUS_USER;
            } else {
                DomainWrap *domain = new DomainWrap(agent, this, domain_ptr, conn);
                io_args->o_domain = domain->GetManagementObject()->getObjectId();
                return STATUS_OK;
            }
        }

        case Node::METHOD_STORAGE_POOL_DEFINE_XML:
        {
            ArgsNodeStorage_pool_define_xml *io_args = (ArgsNodeStorage_pool_define_xml *) &args;
            virStoragePoolPtr pool_ptr;

            pool_ptr = virStoragePoolDefineXML (conn, io_args->i_xml_desc.c_str(), 0);
            if (pool_ptr == NULL) {
                errstr = FORMAT_ERR(conn, "Error defining storage pool using xml description (virStoragePoolDefineXML).");
                return STATUS_USER;
            }

            PoolWrap *pool = new PoolWrap(agent, this, pool_ptr, conn);

            io_args->o_pool = pool->GetManagementObject()->getObjectId();
            return STATUS_OK;

        }
        case Node::METHOD_STORAGE_POOL_CREATE_XML:
        {
            ArgsNodeStorage_pool_create_xml *io_args = (ArgsNodeStorage_pool_create_xml *) &args;
            virStoragePoolPtr pool_ptr;

            pool_ptr = virStoragePoolCreateXML (conn, io_args->i_xml_desc.c_str(), 0);
            if (pool_ptr == NULL) {
                errstr = FORMAT_ERR(conn, "Error creating storage pool using xml description (virStoragePoolCreateXML).");
                return STATUS_USER;
            }

            PoolWrap *pool = new PoolWrap(agent, this, pool_ptr, conn);
            io_args->o_pool = pool->GetManagementObject()->getObjectId();

            return STATUS_OK;
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}


//==============================================================
// Main program
//==============================================================
int main(int argc, char** argv) {
    ManagementAgent::Singleton singleton;
    const char* host = argc>1 ? argv[1] : "127.0.0.1";
    int port = argc>2 ? atoi(argv[2]) : 5672;

    // Create the management agent
    ManagementAgent* agent = singleton.getInstance();

    // Register the schema with the agent
    PackageLibvirt packageInit(agent);

    // Start the agent.  It will attempt to make a connection to the
    // management broker.  The third argument is the interval for sending
    // updates to stats/properties to the broker.  The last is set to 'true'
    // to keep this all single threaded.  Otherwise a second thread would be
    // used to implement methods.
    agent->init(string(host), port, 5, true);

    NodeWrap node(agent, "Libvirt Node");

    node.doLoop();
}


