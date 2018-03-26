#ifndef __RQEUEST_H
#define __RQEUEST_H
#include "protocol.h"

namespace selectsrv {
	typedef struct  {
		protocol _protocol;
		std::string _body;
	}request;

	typedef struct  {
		int32 _fd;	//client fd
		protocol _protocol;
		std::string _body;
	}cli_req;

	static const int32 REQUEST_LEN = sizeof(cli_req);
}

#endif