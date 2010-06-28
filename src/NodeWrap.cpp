
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <syslog.h>
#include <signal.h>

#include "NodeWrap.h"
#include "DomainWrap.h"
#include "PoolWrap.h"
#include "Error.h"

#include "ArgsNodeDomainDefineXML.h"
#include "ArgsNodeStoragePoolCreateXML.h"
#include "ArgsNodeStoragePoolDefineXML.h"
#include "ArgsNodeFindStoragePoolSources.h"

namespace _qmf = qmf::com::redhat::libvirt;

NodeWrap::NodeWrap(ManagementAgent* _agent, string _name) : name(_name), agent(_agent)
{
    virNodeInfo info;
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
        throw -1;
    }

    hostname = virConnectGetHostname(conn);
    if (hostname == NULL) {
        REPORT_ERR(conn, "virConnectGetHostname");
        throw -1;
    }

    hv_type = virConnectGetType(conn);
    if (hv_type == NULL) {
        REPORT_ERR(conn, "virConnectGetType");
        throw -1;
    }

    uri = virConnectGetURI(conn);
    if (uri == NULL) {
        REPORT_ERR(conn, "virConnectGetURI");
        throw -1;
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

    ret = virNodeGetInfo(conn, &info);
    if (ret < 0) {
        REPORT_ERR(conn, "virNodeGetInfo");
        memset((void *) &info, sizeof(info), 1);
    }

    mgmtObject = new _qmf::Node(agent, this, hostname, uri, libvirt_version, api_version, hv_version, hv_type,
                                info.model, info.memory, info.cpus, info.mhz, info.nodes, info.sockets,
                                info.cores, info.threads);
    agent->addObject(mgmtObject);
}

NodeWrap::~NodeWrap()
{
    /* Go through our list of pools and destroy them all! MOOOHAHAHA */
    for (std::vector<PoolWrap*>::iterator iter = pools.begin(); iter != pools.end();) {
        delete(*iter);
        iter = pools.erase(iter);
    }

    /* Same for domains.. */
    for (std::vector<DomainWrap*>::iterator iter = domains.begin(); iter != domains.end();) {
        delete (*iter);
        iter = domains.erase(iter);
    }

    mgmtObject->resourceDestroy();
}

void NodeWrap::syncDomains()
{
    /* Sync up with domains that are defined but not active. */
    int maxname = virConnectNumOfDefinedDomains(conn);
    if (maxname < 0) {
        REPORT_ERR(conn, "virConnectNumOfDefinedDomains");
        return;
    } else {
        char **dnames;
        dnames = (char **) malloc(sizeof(char *) * maxname);

        if ((maxname = virConnectListDefinedDomains(conn, dnames, maxname)) < 0) {
            REPORT_ERR(conn, "virConnectListDefinedDomains");
            free(dnames);
            return;
        }


        for (int i = 0; i < maxname; i++) {
            virDomainPtr domain_ptr;

            bool found = false;
            for (std::vector<DomainWrap*>::iterator iter = domains.begin();
                    iter != domains.end(); iter++) {
                if ((*iter)->domain_name == dnames[i]) {
                    found = true;
                    break;
                }
            }

            if (found) {
                continue;
            }

            domain_ptr = virDomainLookupByName(conn, dnames[i]);
            if (!domain_ptr) {
                REPORT_ERR(conn, "virDomainLookupByName");
            } else {
                DomainWrap *domain;
                try {
                    domain = new DomainWrap(agent, this, domain_ptr, conn);
                    printf("Created new domain: %s, ptr is %p\n", dnames[i], domain_ptr);
                    domains.push_back(domain);
                } catch (int i) {
                    printf ("Error constructing domain\n");
                    REPORT_ERR(conn, "constructing domain.");
                    delete domain;
                }
            }
        }
        for (int i = 0; i < maxname; i++) {
            free(dnames[i]);
        }

        free(dnames);
    }

    /* Go through all the active domains */
    int maxids = virConnectNumOfDomains(conn);
    if (maxids < 0) {
        REPORT_ERR(conn, "virConnectNumOfDomains");
        return;
    } else {
        int *ids;
        ids = (int *) malloc(sizeof(int *) * maxids);

        if ((maxids = virConnectListDomains(conn, ids, maxids)) < 0) {
            printf("Error getting list of defined domains\n");
            return;
        }

        for (int i = 0; i < maxids; i++) {
            virDomainPtr domain_ptr;
            char dom_uuid[VIR_UUID_STRING_BUFLEN];

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

            DomainWrap *domain;
            try {
                domain = new DomainWrap(agent, this, domain_ptr, conn);
                printf("Created new domain: %d, ptr is %p\n", ids[i], domain_ptr);
                domains.push_back(domain);
            } catch (int i) {
                printf ("Error constructing domain\n");
                REPORT_ERR(conn, "constructing domain.");
                delete domain;
            }
        }

        free(ids);
    }

    /* Go through our list of domains and ensure that they still exist. */
    for (std::vector<DomainWrap*>::iterator iter = domains.begin(); iter != domains.end();) {

        printf("verifying domain %s\n", (*iter)->domain_name.c_str());
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
        printf("Creating new pool: %s, ptr is %p\n", pool_name, pool_ptr);
        PoolWrap *pool;
        try {
            pool = new PoolWrap(agent, this, pool_ptr, conn);
            printf("Created new pool: %s, ptr is %p\n", pool_name, pool_ptr);
            pools.push_back(pool);
        } catch (int i) {
            printf ("Error constructing pool\n");
            REPORT_ERR(conn, "constructing pool.");
            delete pool;
        }
    }
}

void NodeWrap::syncPools()
{
    int maxname;

    maxname = virConnectNumOfStoragePools(conn);
    if (maxname < 0) {
        REPORT_ERR(conn, "virConnectNumOfStroagePools");
        return;
    } else {
        char **names;
        names = (char **) malloc(sizeof(char *) * maxname);

        if ((maxname = virConnectListStoragePools(conn, names, maxname)) < 0) {
            REPORT_ERR(conn, "virConnectListStoragePools");
            return;
        }

        for (int i = 0; i < maxname; i++) {
            checkPool(names[i]);
            free(names[i]);
        }
        free(names);
    }

    maxname = virConnectNumOfDefinedStoragePools(conn);
    if (maxname < 0) {
        REPORT_ERR(conn, "virConnectNumOfDefinedStoragePools");
        return;
    } else {
        char **names;
        names = (char **) malloc(sizeof(char *) * maxname);

        if ((maxname = virConnectListDefinedStoragePools(conn, names, maxname)) < 0) {
            REPORT_ERR(conn, "virConnectListDefinedStoragePools");
            return;
        }

        for (int i = 0; i < maxname; i++) {
            checkPool(names[i]);
            free(names[i]);
        }

        free(names);
    }

    /* Go through our list of pools and ensure that they still exist. */
    for (std::vector<PoolWrap*>::iterator iter = pools.begin(); iter != pools.end();) {

        printf ("Verifying pool %s\n", (*iter)->pool_name.c_str());
        virStoragePoolPtr ptr = virStoragePoolLookupByUUIDString(conn, (*iter)->pool_uuid.c_str());
        if (ptr == NULL) {
            printf("Destroying pool %s\n", (*iter)->pool_name.c_str());
            delete(*iter);
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
        int read_fd = agent->getSignalFd();

        // We're using this to check to see if our connection is still good.
        // I don't see any reason this call should fail unless there is some
        // connection problem..
        int maxname = virConnectNumOfDefinedDomains(conn);
        if (maxname < 0) {
            return;
        }

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

        /* Poll agent fd.  If any methods are being made this FD will be ready for reading.  */
        FD_ZERO(&fds);
        FD_SET(read_fd, &fds);

        /* Wait up to three seconds. */
        tv.tv_sec = 3;
        tv.tv_usec = 0;

        retval = select(read_fd + 1, &fds, NULL, NULL, &tv);
        if (retval < 0) {
            fprintf(stderr, "Error in select loop: %s\n", strerror(errno));
            continue;
        }

        if (retval > 0) {
            /* This implements any pending methods. */
            agent->pollCallbacks();
        }

    }
}

Manageable::status_t
NodeWrap::ManagementMethod(uint32_t methodId, Args& args, std::string &errstr)
{
    virDomainPtr domain_ptr;
    cout << "Method Received: " << methodId << endl;
    int ret;

    switch (methodId) {
        case _qmf::Node::METHOD_DOMAINDEFINEXML:
        {
            _qmf::ArgsNodeDomainDefineXML *io_args = (_qmf::ArgsNodeDomainDefineXML *) &args;
            domain_ptr = virDomainDefineXML(conn, io_args->i_xmlDesc.c_str());
            if (!domain_ptr) {
                errstr = FORMAT_ERR(conn, "Error creating domain using xml description (virDomainDefineXML).", &ret);
                return STATUS_USER + ret;
            } else {
                // Now we have to check to see if this domain is actually new or not, because it's possible that
                // one already exists with this name/description and we just replaced it.. *ugh*
                for (std::vector<DomainWrap*>::iterator iter = domains.begin(); iter != domains.end();) {
                    if (strcmp((*iter)->domain_name.c_str(), virDomainGetName(domain_ptr)) == 0) {
                        // We're just replacing an existing domain, however I'm pretty sure the
                        // old domain pointer becomes invalid at this point, so we should destroy
                        // the old domain reference.  The other option would be to replace it and
                        // keep the object valid.. not sure which is better.
                        printf("Old domain already exists, removing it in favor of new object.");
                        delete(*iter);
                        iter = domains.erase(iter);
                    } else {
                        iter++;
                    }
                }

                DomainWrap *domain;
                try {
                    domain = new DomainWrap(agent, this, domain_ptr, conn);
                    domains.push_back(domain);
                    io_args->o_domain = domain->GetManagementObject()->getObjectId();
                } catch (int i) {
                    delete domain;
                    errstr = FORMAT_ERR(conn, "Error constructing domain object in virDomainDefineXML.", &ret);
                    return STATUS_USER + i;
                }

                return STATUS_OK;
            }
        }

        case _qmf::Node::METHOD_STORAGEPOOLDEFINEXML:
        {
            _qmf::ArgsNodeStoragePoolDefineXML *io_args = (_qmf::ArgsNodeStoragePoolDefineXML *) &args;
            virStoragePoolPtr pool_ptr;

            pool_ptr = virStoragePoolDefineXML (conn, io_args->i_xmlDesc.c_str(), 0);
            if (pool_ptr == NULL) {
                errstr = FORMAT_ERR(conn, "Error defining storage pool using xml description (virStoragePoolDefineXML).", &ret);
                return STATUS_USER + ret;
            }

            PoolWrap *pool;
            try {
                pool = new PoolWrap(agent, this, pool_ptr, conn);
                pools.push_back(pool);
                io_args->o_pool = pool->GetManagementObject()->getObjectId();
            } catch (int i) {
                delete pool;
                errstr = FORMAT_ERR(conn, "Error constructing pool object in virStoragePoolDefineXML.", &ret);
                return STATUS_USER + i;
            }
            return STATUS_OK;

        }
        case _qmf::Node::METHOD_STORAGEPOOLCREATEXML:
        {
            _qmf::ArgsNodeStoragePoolCreateXML *io_args = (_qmf::ArgsNodeStoragePoolCreateXML *) &args;
            virStoragePoolPtr pool_ptr;

            pool_ptr = virStoragePoolCreateXML (conn, io_args->i_xmlDesc.c_str(), 0);
            if (pool_ptr == NULL) {
                errstr = FORMAT_ERR(conn, "Error creating storage pool using xml description (virStoragePoolCreateXML).", &ret);
                return STATUS_USER + ret;
            }

            PoolWrap *pool;
            try {
                pool = new PoolWrap(agent, this, pool_ptr, conn);
                pools.push_back(pool);
                io_args->o_pool = pool->GetManagementObject()->getObjectId();
            } catch (int i) {
                delete pool;
                errstr = FORMAT_ERR(conn, "Error constructing pool object in virStoragePoolCreateXML.", &ret);
                return STATUS_USER + i;
            }

            return STATUS_OK;
        }
        case _qmf::Node::METHOD_FINDSTORAGEPOOLSOURCES:
        {
            _qmf::ArgsNodeFindStoragePoolSources *io_args = (_qmf::ArgsNodeFindStoragePoolSources *) &args;
            char *xml_result;

            xml_result = virConnectFindStoragePoolSources(conn, io_args->i_type.c_str(), io_args->i_srcSpec.c_str(), 0);
            if (xml_result == NULL) {
                errstr = FORMAT_ERR(conn, "Error creating storage pool using xml description (virStoragePoolCreateXML).", &ret);
                return STATUS_USER + ret;
            }

            io_args->o_xmlDesc = xml_result;
            free(xml_result);

            return STATUS_OK;
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}

static void
print_usage()
{
    printf("Usage:\tlibvirt-qpid <options>\n");
    printf("\t-d | --daemon     run as a daemon.\n");
    printf("\t-h | --help       print this help message.\n");
    printf("\t-b | --broker     specify broker host name..\n");
    printf("\t-g | --gssapi     force GSSAPI authentication.\n");
    printf("\t-u | --username   username to use for authentication purproses.\n");
    printf("\t-s | --service    service name to use for authentication purproses.\n");
    printf("\t-p | --port       specify broker port.\n");
}


//==============================================================
// Main program
//==============================================================
int main(int argc, char** argv) {
    int arg;
    int idx = 0;
    bool verbose = false;
    bool daemonize = false;
    bool gssapi = false;
    char *host = NULL;
    char *username = NULL;
    char *service = NULL;
    int port = 5672;

    struct option opt[] = {
        {"help", 0, 0, 'h'},
        {"daemon", 0, 0, 'd'},
        {"broker", 1, 0, 'b'},
        {"gssapi", 0, 0, 'g'},
        {"username", 1, 0, 'u'},
        {"service", 1, 0, 's'},
        {"port", 1, 0, 'p'},
        {0, 0, 0, 0}
    };

    while ((arg = getopt_long(argc, argv, "hdb:gu:s:p:", opt, &idx)) != -1) {
        switch (arg) {
            case 'h':
            case '?':
                print_usage();
                exit(0);
                break;
            case 'd':
                daemonize = true;
                break;
            case 'v':
                verbose = true;
                break;
            case 's':
                if (optarg) {
                    service = strdup(optarg);
                } else {
                    print_usage();
                    exit(1);
                }
                break;
            case 'u':
                if (optarg) {
                    username = strdup(optarg);
                } else {
                    print_usage();
                    exit(1);
                }
                break;
            case 'g':
                gssapi = true;
                break;
            case 'p':
                if (optarg) {
                    port = atoi(optarg);
                } else {
                    print_usage();
                    exit(1);
                }
                break;
            case 'b':
                if (optarg) {
                    host = strdup(optarg);
                } else {
                    print_usage();
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "unsupported option '-%c'.  See --help.\n", arg);
                print_usage();
                exit(0);
            break;
        }
    }

    if (daemonize == true) {
        if (daemon(0, 0) < 0) {
            fprintf(stderr, "Error daemonizing: %s\n", strerror(errno));
            exit(1);
        }
    }
    openlog("libvirt-qpid", 0, LOG_DAEMON);

    // This prevents us from dying if libvirt disconnects.
    signal(SIGPIPE, SIG_IGN);

    // Create the management agent
    ManagementAgent::Singleton singleton;
    ManagementAgent* agent = singleton.getInstance();

    // Register the schema with the agent
    _qmf::Package packageInit(agent);

    // Start the agent.  It will attempt to make a connection to the
    // management broker.  The third argument is the interval for sending
    // updates to stats/properties to the broker.  The last is set to 'true'
    // to keep this all single threaded.  Otherwise a second thread would be
    // used to implement methods.

    qpid::management::ConnectionSettings settings;
    settings.host = host ? host : "127.0.0.1";
    settings.port = port;

    if (username != NULL) {
        settings.username = username;
    }
    if (service != NULL) {
        settings.service = service;
    }
    if (gssapi == true) {
        settings.mechanism = "GSSAPI";
    }

    agent->setName("Red Hat", "libvirt-qpid");
    agent->init(settings, 3, true);

    while(true) {
        try {
            NodeWrap node(agent, "Libvirt Node");
            node.doLoop();
        } catch (int err) {
        }
        sleep(10);
    }
}


