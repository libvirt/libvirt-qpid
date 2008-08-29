
#include "NodeWrap.h"
#include "DomainWrap.h"

#include "ArgsDomainMigrate.h"
#include "ArgsDomainRestore.h"
#include "ArgsDomainSave.h"
#include "ArgsDomainXml_desc.h"

DomainWrap::DomainWrap(ManagementAgent *agent, NodeWrap *parent, virDomainPtr domain_ptr, 
                         virConnectPtr connect, std::string uuid, std::string name)
{
    domain = new Domain(agent, this, parent, uuid, name);
    agent->addObject(domain);
    conn = connect;
    dom = domain_ptr;
}

void
DomainWrap::update()
{
    virDomainInfo info;
    int ret;

    ret = virDomainGetInfo(dom, &info);
    if (ret < 0) {
        printf("Domain get info failed, domain dead?? - ptr %p\n", dom);
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

        domain->set_num_vcpus(info.nrVirtCpu);
        domain->set_maximum_memory(info.maxMem);
        domain->set_memory(info.memory);
        domain->set_cpu_time(info.cpuTime);
    }

    ret = virDomainGetID(dom);
    // Just set it directly, if there's an error, -1 will be right.
    domain->set_id(ret);
}

Manageable::status_t 
DomainWrap::ManagementMethod(uint32_t methodId, Args& args)
{
    Mutex::ScopedLock _lock(vectorLock);
    cout << "Method Received: " << methodId << endl;
    int ret;

    switch (methodId) {
        case Domain::METHOD_CREATE:
            ret = virDomainCreate(dom);
            update();
            if (ret < 0) {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;

        case Domain::METHOD_DESTROY:
            ret = virDomainDestroy(dom);
            update();
            if (ret < 0) {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;

        case Domain::METHOD_UNDEFINE:
            ret = virDomainUndefine(dom);
            update();
            if (ret < 0) {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;

        case Domain::METHOD_SUSPEND:
            ret = virDomainSuspend(dom);
            update();
            if (ret < 0) {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;

        case Domain::METHOD_RESUME:
            ret = virDomainResume(dom);
            update();
            if (ret < 0) {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;

        case Domain::METHOD_SAVE:
            {
                ArgsDomainSave *ioArgs = (ArgsDomainSave *) &args;

                ret = virDomainSave(dom, ioArgs->i_filename.c_str());
                update();
                if (ret < 0) {
                    return STATUS_INVALID_PARAMETER;
                }
                return STATUS_OK;
            }

        case Domain::METHOD_RESTORE:
            {
                ArgsDomainRestore *ioArgs = (ArgsDomainRestore *) &args;

                ret = virDomainRestore(conn, ioArgs->i_filename.c_str());
                update();
                if (ret < 0) {
                    return STATUS_INVALID_PARAMETER;
                }
                return STATUS_OK;
            }

        case Domain::METHOD_SHUTDOWN:
            ret = virDomainShutdown(dom);
            update();
            if (ret < 0) {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;

        case Domain::METHOD_REBOOT:
            ret = virDomainReboot(dom, 0);
            update();
            if (ret < 0) {
                return STATUS_INVALID_PARAMETER;
            }
            return STATUS_OK;

        case Domain::METHOD_XML_DESC:
            {
                ArgsDomainXml_desc *ioArgs = (ArgsDomainXml_desc *) &args;
                char *desc;
                desc = virDomainGetXMLDesc(dom, VIR_DOMAIN_XML_SECURE | VIR_DOMAIN_XML_INACTIVE);
                if (desc) {
                    ioArgs->o_description = desc;
                } else {
                    return STATUS_INVALID_PARAMETER;
                }
                return STATUS_OK; 
            }

        case Domain::METHOD_MIGRATE:
            {
                ArgsDomainMigrate *ioArgs = (ArgsDomainMigrate *) &args;

                // ret = virDomainMigrate(dom, ioArgs->i_filename.c_str());
                return STATUS_NOT_IMPLEMENTED;
            }
    }

    return STATUS_NOT_IMPLEMENTED;
}


