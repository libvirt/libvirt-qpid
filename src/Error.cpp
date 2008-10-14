
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>

#include <iostream>
#include <sstream>
#include <syslog.h>

#include "Error.h"

static const char *
getErrorSource(virErrorPtr err)
{
    switch(err->domain) {
        case VIR_FROM_NONE:
            return "none";
        case VIR_FROM_XEN:
            return "xen";
        case VIR_FROM_XEND:
            return "xend";
        case VIR_FROM_XENSTORE:
            return "xenstore";
        case VIR_FROM_SEXPR:
            return "sexpr";
        case VIR_FROM_XML:
            return "xml";
        case VIR_FROM_DOM:
            return "domain";
        case VIR_FROM_RPC:
            return "rpc";
        case VIR_FROM_PROXY:
            return "proxy";
        case VIR_FROM_CONF:
            return "conf";
        case VIR_FROM_QEMU:
            return "qemu";
        case VIR_FROM_NET:
            return "net";
        case VIR_FROM_TEST:
            return "test";
        case VIR_FROM_REMOTE:
            return "remote";
        case VIR_FROM_OPENVZ:
            return "openvz";
        case VIR_FROM_XENXM:
            return "xenxm";
        case VIR_FROM_STATS_LINUX:
            return "stats linux";
        case VIR_FROM_LXC:
            return "lxc";
        case VIR_FROM_STORAGE:
            return "storage";
    }
    return "unknown";
}


std::string
formatError(virConnectPtr conn, const char *msg, int *err_ret, const char *function, int line, const char *file)
{
    virErrorPtr err;
    std::ostringstream errmsg;

    *err_ret = -1;

    if (!conn) {
        errmsg << "NULL connection: " << msg << " " << file << " " << function << " " << line;
        return errmsg.str();
    }
    err = virConnGetLastError(conn);
    if (!err) {
        errmsg << "NULL error handle: " << msg << " " << file << " " << function << " " << line;
        return errmsg.str();
    }

    *err_ret = err->code;
    errmsg << "Error: " << msg << " Subsystem " << getErrorSource(err) << ": ";
    errmsg << err->message;
    errmsg << " in " << file << ":" << function << ":" << line;

    return errmsg.str();
}

void
reportError(virConnectPtr conn, const char *msg, const char *function, int line, const char *file)
{
    std::string err;
    int err_ret;

    err = formatError(conn, msg, &err_ret, function, line, file);
    std::cout << "\n" << err << " code: " << err_ret << "\n";

    syslog(LOG_ERR, "%s code: %d", err.c_str(), err_ret);
}


