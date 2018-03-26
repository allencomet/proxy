#ifndef __REQUEST_PARSER_H
#define __REQUEST_PARSER_H

#include "protocol.h"
#include "request.h"

namespace selectsrv {
	enum parser_status {
		ERROR,
		INPROGRESS,
		HASDONE,
		HASLEFT
	};
	class request_parser {
	public:
		request_parser();
		~request_parser();

		parser_status parser(StreamBuf &, cli_req &);
	private:
		DISALLOW_COPY_AND_ASSIGN(request_parser);
	};
}

#endif