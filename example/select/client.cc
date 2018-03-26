#include "client.h"

namespace selectsrv {
	bool client::handle_request() {
		safe::MutexGuard g(_rmutex);
		errorlog::err_msg("handle request %d bytes data: %s", _rbuf.size(), _rbuf.to_string().c_str());
		for (;;) {//handle all complete packet if has over one packet
			cli_req req;
			req._fd = _fd;
			bool bre = false;
			switch (_request_parser.parser(_rbuf, req)) {
			case ERROR:
				bre = true;
				break;
			case INPROGRESS:
				bre = true;
				break;
			case HASDONE:
				//add request packet to queue,asynchronous handle
				_request_handler.handle_request(req);
				bre = true;
				break;
			case HASLEFT:
				//add request packet to queue,asynchronous handle
				_request_handler.handle_request(req);
				break;
			default:
				break;
			}

			if (bre) break;
		}

		return true;
	}
}