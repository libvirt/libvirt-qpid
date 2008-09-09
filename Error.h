
#define REPORT_ERR(conn,msg) reportError(conn, msg, __func__, __LINE__, __FILE__);

#define FORMAT_ERR(conn,msg) formatError(conn, msg, __func__, __LINE__, __FILE__);

void
reportError(virConnectPtr conn, const char *msg, const char *function, int line, const char *file);

std::string
formatError(virConnectPtr conn, const char *msg, const char *function, int line, const char *file);

