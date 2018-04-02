#ifndef __PROXY_RPC_H
#define __PROXY_RPC_H

#include "../../util/util.h"

DEC_uint32(thcount);		//the number of thread in server
DEC_string(srvpath);		//ipc server bind address
DEC_string(srvip);			//rpc server bind address
DEC_uint32(srvport);		//server listen port

namespace proxy {
	namespace rpc {

		class NetServer {
		public:
			typedef boost::shared_ptr<safe::Thread> ThreadPtr;

			explicit NetServer(const std::string &ip, const int16 &port);
			~NetServer() {
				stop();
			}

			void run();

			void stop() {
				for (std::vector<ThreadPtr>::iterator it = _threads.begin();
					it != _threads.end(); ++it) {
					if (*it) {
						(*it)->cancel();
						(*it)->join();
					}
				}
			}
		private:
			void worker(int32 listenfd);

			std::string _ip;//server ip
			int16 _port;//server port
			int32 _listenfd;
			std::vector<ThreadPtr> _threads;

			DISALLOW_COPY_AND_ASSIGN(NetServer);
		};

	}//namespace rpc
}//namespace proxy

#endif