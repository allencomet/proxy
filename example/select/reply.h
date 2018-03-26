#ifndef __REPLY_H
#define __REPLY_H

#include "protocol.h"

namespace selectsrv {
	struct reply {
		protocol _protocol;
		std::string _body;
	};

	struct cli_rep {
		int32 _fd;
		reply _rep;
	};
}

#endif