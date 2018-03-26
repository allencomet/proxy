#ifndef __REQUEST_HANDLER_H
#define __REQUEST_HANDLER_H

#include "../util/util.h"

#include "request.h"
#include "reply.h"

namespace selectsrv {
	struct reply;
	
	class request_handler {	
	public:
		typedef boost::shared_ptr<safe::Thread> ThreadPtr;

		request_handler(int32 size=5);
		~request_handler();

		void handle_request(const cli_req &req) {
			_queue.push(req);
		}

	private:
		void worker();
		void handle(const cli_req &);

		std::vector<ThreadPtr> _threads;
		safe::block_queue<cli_req> _queue;

		DISALLOW_COPY_AND_ASSIGN(request_handler);
	};
}

#endif