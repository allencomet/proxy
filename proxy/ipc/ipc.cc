#include "ipc.h"

#include "dispatcher_ipc.h"

namespace proxy {
	namespace ipc {

		LocalServer::LocalServer(const std::string &path)
			:_path(path),
			_listenfd(0) {
			_listenfd = Dispatcher::init_ipc_socket(path);
			for (int16 i = 0; i < ::FLG_thcount; ++i) {
				ThreadPtr ptr(new safe::Thread(boost::bind(&LocalServer::worker, this, _listenfd)));
				_threads.push_back(ptr);
			}
		}

		void LocalServer::run() {
			for (std::vector<ThreadPtr>::iterator it = _threads.begin();
				it != _threads.end(); ++it) {
				(*it)->start();
			}

			for (;;) {
				sys::sleep(60);
			}
		}

		void LocalServer::worker(int32 listenfd) {
			LOG << "LocalServer listen on path: " << _path;
			Dispatcher netcore(_path);
			netcore.init(listenfd);
			netcore.run();
		}

	}//namespace ipc
}//namespace proxy
