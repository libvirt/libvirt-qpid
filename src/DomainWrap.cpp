
#include "NodeWrap.h"
#include "DomainWrap.h"
#include "Error.h"

#include "ArgsDomainMigrate.h"
#include "ArgsDomainRestore.h"
#include "ArgsDomainSave.h"
#include "ArgsDomainGetXMLDesc.h"

namespace _qmf = qmf::com::redhat::libvirt;

DomainWrap::DomainWrap(ManagementAgent *agent, NodeWrap *parent,
                       virDomainPtr _domain_ptr, virConnectPtr _conn) :
                       domain_ptr(_domain_ptr), conn(_conn)
{
    char dom_uuid[VIR_UUID_STRING_BUFLEN];

    domain = NULL;

    if (virDomainGetUUIDString(domain_ptr, dom_uuid) < 0) {
        REPORT_ERR(conn, "DomainWrap: Unable to get UUID string of domain.");
        throw 1;
    }

    domain_uuid = dom_uuid;

    const char *dom_name = virDomainGetName(domain_ptr);
    if (!dom_name) {
        REPORT_ERR(conn, "Unable to get domain name!\n");
        throw 1;
    }

    domain_name = dom_name;

    domain = new _qmf::Domain(agent, this, parent, domain_uuid, domain_name);
    printf("Creating new domain object for %s\n", domain_name.c_str());
    agent->addObject(domain);
}

DomainWrap::~DomainWrap()
{
    if (domain) {
        domain->resourceDestroy();
    }
    virDomainFree(domain_ptr);
}


void
DomainWrap::update()
{
    virDomainInfo info;
    int ret;

    ret = virDomainGetInfo(domain_ptr, &info);
    if (ret < 0) {
        REPORT_ERR(conn, "Domain get info failed.");
        /* Next domainSync() will take care of this in the node wrapper if the domain is
         * indeed dead. */
        return;
    } else {
        switch (info.state) {

            case VIR_DOMAIN_NOSTATE:
                domain->set_state("nostate");
                break;
            case VIR_DOMAIN_RUNNING:
                domain->set_state("running");
                break;
            case VIR_DOMAIN_BLOCKED:
                domain->set_state("blocked");
                break;
            case VIR_DOMAIN_PAUSED:
                domain->set_state("paused");
                break;
            case VIR_DOMAIN_SHUTDOWN:
                domain->set_state("shutdown");
                break;
            case VIR_DOMAIN_SHUTOFF:
                domain->set_state("shutoff");
                break;
            case VIR_DOMAIN_CRASHED:
                domain->set_state("crashed");
                break;
        }

        domain->set_numVcpus(info.nrVirtCpu);
        domain->set_maximumMemory(info.maxMem);
        domain->set_memory(info.memory);
        domain->set_cpuTime(info.cpuTime);
    }

    ret = virDomainGetID(domain_ptr);
    // Just set it directly, if there's an error, -1 will be right.
    domain->set_id(ret);

    if (ret > 0) {
        domain->set_active("true");
    } else {
        domain->set_active("false");
    }
    domain->set_id(ret);
}

Manageable::status_t
DomainWrap::ManagementMethod(uint32_t methodId, Args& args, std::string &errstr)
{
    int ret;

    switch (methodId) {
        case _qmf::Domain::METHOD_CREATE:
            ret = virDomainCreate(domain_ptr);
            update();
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error creating new domain (virDomainCreate).", &ret);
                return STATUS_USER + ret;
            }
            return STATUS_OK;

        case _qmf::Domain::METHOD_DESTROY:
            ret = virDomainDestroy(domain_ptr);
            update();
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error destroying domain (virDomainDestroy).", &ret);
                return STATUS_USER + ret;
            }

            return STATUS_OK;

        case _qmf::Domain::METHOD_UNDEFINE:
            ret = virDomainUndefine(domain_ptr);

            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error undefining domain (virDomainUndefine).", &ret);
                return STATUS_USER + ret;
            }

            /* We now wait for domainSync() to clean this up. */
            return STATUS_OK;

        case _qmf::Domain::METHOD_SUSPEND:
            ret = virDomainSuspend(domain_ptr);
            update();
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error suspending domain (virDomainSuspend).", &ret);
                return STATUS_USER + ret;
            }
            return STATUS_OK;

        case _qmf::Domain::METHOD_RESUME:
            ret = virDomainResume(domain_ptr);
            update();
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error resuming domain (virDomainResume).", &ret);
                return STATUS_USER + ret;
            }
            return STATUS_OK;

        case _qmf::Domain::METHOD_SAVE:
            {
                _qmf::ArgsDomainSave *io_args = (_qmf::ArgsDomainSave *) &args;

                ret = virDomainSave(domain_ptr, io_args->i_filename.c_str());
                if (ret < 0) {
                    errstr = FORMAT_ERR(conn, "Error saving domain (virDomainSave).", &ret);
                    return STATUS_USER + ret;
                }
                return STATUS_OK;
            }

        case _qmf::Domain::METHOD_RESTORE:
            {
                _qmf::ArgsDomainRestore *io_args = (_qmf::ArgsDomainRestore *) &args;

                ret = virDomainRestore(conn, io_args->i_filename.c_str());
                update();
                if (ret < 0) {
                    errstr = FORMAT_ERR(conn, "Error restoring domain (virDomainRestore).", &ret);
                    return STATUS_USER + ret;
                }
                return STATUS_OK;
            }

        case _qmf::Domain::METHOD_SHUTDOWN:
            ret = virDomainShutdown(domain_ptr);
            update();
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error shutting down domain (virDomainShutdown).", &ret);
                return STATUS_USER + ret;
            }
            return STATUS_OK;

        case _qmf::Domain::METHOD_REBOOT:
            ret = virDomainReboot(domain_ptr, 0);
            update();
            if (ret < 0) {
                errstr = FORMAT_ERR(conn, "Error rebooting domain (virDomainReboot).", &ret);
                return STATUS_USER + ret;
            }
            return STATUS_OK;

        case _qmf::Domain::METHOD_GETXMLDESC:
            {
                _qmf::ArgsDomainGetXMLDesc *io_args = (_qmf::ArgsDomainGetXMLDesc *) &args;
                char *desc;
                desc = virDomainGetXMLDesc(domain_ptr, VIR_DOMAIN_XML_SECURE | VIR_DOMAIN_XML_INACTIVE);
                if (desc) {
                    io_args->o_description = desc;
                } else {
                    errstr = FORMAT_ERR(conn, "Error getting XML description of domain(virDomainGetXMLDesc).", &ret);
                    return STATUS_USER + ret;
                }
                return STATUS_OK;
            }

        case _qmf::Domain::METHOD_MIGRATE:
            {
                virConnectPtr dest_conn;
                virDomainPtr rem_dom;
                _qmf::ArgsDomainMigrate *io_args = (_qmf::ArgsDomainMigrate *) &args;

                // This is actually quite broken.  Most setups won't allow unauthorized connection
                // from one node to another directly like this.
                dest_conn = virConnectOpen(io_args->i_destinationUri.c_str());
                if (!dest_conn) {
                    errstr = FORMAT_ERR(dest_conn, "Unable to connect to remote system for migration: virConnectOpen", &ret);
                    return STATUS_USER + ret;
                }

                const char *new_dom_name = NULL;
                if (io_args->i_newDomainName.size() > 0) {
                    new_dom_name = io_args->i_newDomainName.c_str();
                }

                const char *uri = NULL;
                if (io_args->i_uri.size() > 0) {
                    uri = io_args->i_uri.c_str();
                }

                printf ("calling migrate, new_dom_name: %s, uri: %s, flags: %d (live is %d)\n",
                        new_dom_name ? new_dom_name : "NULL",
                        uri ? uri : "NULL",
                        io_args->i_flags,
                        VIR_MIGRATE_LIVE);

                rem_dom = virDomainMigrate(domain_ptr, dest_conn, io_args->i_flags,
                                           new_dom_name,
                                           uri,
                                           io_args->i_bandwidth);

                virConnectClose(dest_conn);

                if (!rem_dom) {
                    errstr = FORMAT_ERR(conn, "virDomainMigrate", &ret);
                    return STATUS_USER + ret;
                }
                virDomainFree(rem_dom);

                return STATUS_OK;
            }
    }

    return STATUS_NOT_IMPLEMENTED;
}


