#include "proxy.h"

DEF_uint32(thcount, 4, "default create 4 threads to handle request from client");
DEF_string(srvpath, "./mastersrv", "default ipc server bind path");
DEF_string(srvip, "*", "default rpc server bind ip");
DEF_uint32(srvport, 8080, "default server listen on 8080"); 
