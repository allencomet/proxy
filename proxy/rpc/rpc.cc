#include "rpc.h"

#include "dispatcher_rpc.h"

namespace proxy {
	namespace rpc {
		NetServer::NetServer(const std::string &ip, const int16 &port)
			:_ip(ip),
			_port(port) {
			_listenfd = Dispatcher::init_tcp_socket(ip, port);

			for (int16 i = 0; i < ::FLG_thcount; ++i) {
				ThreadPtr ptr(new safe::Thread(boost::bind(&NetServer::worker, this, _listenfd)));
				_threads.push_back(ptr);
			}
		}

		void NetServer::run() {
			for (std::vector<ThreadPtr>::iterator it = _threads.begin();
				it != _threads.end(); ++it) {
				(*it)->start();
			}

			for (;;) {
				sys::sleep(60);
			}
		}

		void NetServer::worker(int32 listenfd) {
			LOG << "NetServer listen on port: " << _port;
			Dispatcher netcore(_ip, _port);
			netcore.init(listenfd);
			netcore.run();
		}

	}//namespace rpc
}//namespace proxy