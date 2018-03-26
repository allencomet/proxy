#ifndef __CLIENT_H
#define __CLIENT_H

#include "../util/util.h"

#include "request_handler.h"
#include "request_parser.h"

namespace selectsrv{
	class client
	{
	public:
		client(int32 fd, request_handler &request_handler, request_parser &request_parser)
			:_fd(fd),
			_request_handler(request_handler),
			_request_parser(request_parser){
		}

		~client(){}

		inline std::string userid() const {
			return _userid;
		}

		inline int32 fd() const {
			return _fd;
		}

		void rdata(const void* data, uint32 size) {
			safe::MutexGuard g(_rmutex);
			_rbuf.append(data, size);
		}

		bool handle_request();

	private:
		StreamBuf& rbuf() {
			return _rbuf;
		}

		StreamBuf& wbuf() {
			return _wbuf;
		}
	private:
		int32 _fd;
		std::string _userid;

		request_handler &_request_handler;
		request_parser &_request_parser;

		safe::Mutex _rmutex;
		StreamBuf _rbuf;

		safe::Mutex _wmutex;
		StreamBuf _wbuf;
		DISALLOW_COPY_AND_ASSIGN(client);
	};


}

#endif