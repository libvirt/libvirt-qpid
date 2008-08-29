
#include <string.h>

#include "NodeWrap.h"
#include "DomainWrap.h"

#include "ArgsNodeDefine_domain_xml.h"


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
        printf ("Error connecting!\n");
        exit(1);
    }

    hostname = virConnectGetHostname(conn);
    if (hostname == NULL) {
        printf ("Failed to get hostname\n");
        exit(1);
    }

    hv_type = virConnectGetType(conn);
    if (hv_type == NULL) {
        printf ("Failed to get HV type\n");
        exit(1);
    }

    uri = virConnectGetURI(conn);
    if (uri == NULL) {
        printf ("Failed to get uri\n");
        exit(1);
    }

    ret = virGetVersion(&libvirt_v, hv_type, &api_v);
    if (ret < 0) {
        printf ("Unable to get version info\n");
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
        printf ("Unable to get HV version info\n");
    } else {
        major = hv_v / 1000000;
        hv_v %= 1000000;
        minor = hv_v / 1000;
        rel = hv_v % 1000;
        snprintf(hv_version, sizeof(hv_version), "%d.%d.%d", major, minor, rel);
    }

    mgmtObject = new Node(agent, this, hostname, uri, libvirt_version, api_version, hv_version, hv_type);
    agent->addObject(mgmtObject);

    syncDomains();

}

void NodeWrap::syncDomains()
{

    /* Sync up with domains that are defined but not active. */
    int maxname = virConnectNumOfDefinedDomains(conn);
    if (maxname < 0) {
        printf("Error getting max domain count\n");
        exit(1);
    } else {
        char *names[maxname];

        if ((maxname = virConnectListDefinedDomains(conn, names, maxname)) < 0) {
            printf("Error getting list of defined domains\n");
            exit(1);
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
                printf ("Unable to get domain ptr for domain name %s\n", names[i]);
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
        printf("Error getting max domain id count\n");
        exit(1);
    } else {
        int ids[maxids];

        if ((maxids = virConnectListDomains(conn, ids, maxids)) < 0) {
            printf("Error getting list of defined domains\n");
            exit(1);
        }

        for (int i = 0; i < maxids; i++) {
            virDomainPtr domain_ptr;
            char dom_uuid[VIR_UUID_BUFLEN];

            domain_ptr = virDomainLookupByID(conn, ids[i]);
            if (!domain_ptr) {
                printf("Unable to get domain ptr for domain name %s\n", ids[i]);
                continue;
            }

            if (virDomainGetUUIDString(domain_ptr, dom_uuid) < 0) {
                printf("1: Unable to get UUID string of domain\n");
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
            printf("Destroying domain %s\n", (*iter)->domain_name.c_str());
            delete (*iter);
            iter = domains.erase(iter);
        } else {
            virDomainFree(ptr);
            iter++;
        }
    }
}

void NodeWrap::doLoop()
{
    /* Go through all domains and call update() for each, letting them update
     * information and statistics. */
    while (1) {
        Mutex::ScopedLock _lock(vectorLock);

        agent->pollCallbacks();
        syncDomains();

        for (std::vector<DomainWrap*>::iterator iter = domains.begin();
                iter != domains.end(); iter++) {
            (*iter)->update();
        }

        sleep(15);
    }
}

Manageable::status_t
NodeWrap::ManagementMethod(uint32_t methodId, Args& args)
{
    virDomainPtr domain_ptr;
    Mutex::ScopedLock _lock(vectorLock);
    cout << "Method Received: " << methodId << endl;

    switch (methodId) {
        case Node::METHOD_DEFINE_DOMAIN_XML:
        {
            ArgsNodeDefine_domain_xml *io_args = (ArgsNodeDefine_domain_xml *) &args;
            domain_ptr = virDomainDefineXML(conn, io_args->i_xml_desc.c_str());
            if (!domain_ptr) {
                printf("Error creating new domain from XML\n");
                return STATUS_INVALID_PARAMETER;
            } else {

                qpid::framing::Buffer buffer;

                DomainWrap *domain = new DomainWrap(agent, this, domain_ptr, conn);
                printf("Created new domain.\n");

                domain->GetManagementObject()->getObjectId().encode(buffer);
                // FIXME: 256??
                buffer.getRawData(io_args->o_domain, 256);

                return STATUS_OK;
            }
        }

        //case Node::METHOD_STORAGE_POOL_DEFINE_XML:
        //case Node::METHOD_STORAGE_POOL_CREATE_XML:
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


