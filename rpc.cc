#include "util/util.h"
#include "proxy/proxy.h"

/*
* usage:
*   ./exe -srvip=* -srvport=8080 -log2stderr
*   ./exe -srvpath=mastersrv -log2stderr
* or
*   ./exe --config=xx.conf
*
*/
int main(int argc, char **argv) {
    ccflag::init_ccflag(argc, argv);
    cclog::init_cclog(os::get_process_name());


    proxy::rpc::NetServer srv(::FLG_srvip, ::FLG_srvport);
    srv.run();


    return 0;
}
