
#define REPORT_ERR(conn,msg) reportError(conn, msg, __func__, __LINE__, __FILE__);

#define FORMAT_ERR(conn,msg,err_ret) formatError(conn, msg, err_ret, __func__, __LINE__, __FILE__);

void
reportError(virConnectPtr conn, const char *msg, const char *function, int line, const char *file);

std::string
formatError(virConnectPtr conn, const char *msg, int *err_ret, const char *function, int line, const char *file);

