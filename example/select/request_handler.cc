#include "request_handler.h"

namespace selectsrv {
	
	request_handler::request_handler(int32 size) {
		for (int32 i = 0; i < size; ++i) {
			ThreadPtr thread(new safe::Thread(boost::bind(&request_handler::worker, this)));
			thread->start();
			_threads.push_back(thread);
		}
	}

	request_handler::~request_handler() {
		for (std::vector<ThreadPtr>::iterator it = _threads.begin();
			it != _threads.end(); ++it) {
			(*it)->join();
		}
	}

	//asynchronous handle worker
	void request_handler::worker() {
		errorlog::err_msg("--woker--");
		cli_req req;
		for (;;) {
			if (_queue.pop(req)){
				handle(req);
			}
		}
	}

	void request_handler::handle(const cli_req &req) {
		std::string data;
		switch (req._protocol._func_no) {
		case 10003:
			data = "登录成功";
			break;
		case 10004:
			data = "登出成功";
			break;
		case 10005:
			data = "交易成功";
			break;
		case 10006:
			data = "撤单成功";
			break;
		default:
			break;
		}

		if (!net::writen(req._fd, (const void *)&req._protocol, PROTOCOL_LEN)) {
			errorlog::err_sys("[handle_request]: write data error");
		}

		if (!net::writen(req._fd, (const void *)data.c_str(), data.size())) {
			errorlog::err_sys("[handle_request]: write data error");
		}

		errorlog::err_msg("[handle_request]: user[%s] -- handle | function[%d] | flowno[%d] successful!", 
			req._protocol._ueser_id, req._protocol._func_no, req._protocol._flow_no);
	}
}