#ifndef __PROXY_RPC_REQUEST_HANDLER_H
#define __PROXY_RPC_REQUEST_HANDLER_H

#include "../../util/util.h"
#include "../core/protocol.h"

namespace proxy {

	class ConnManager;
	class Connection;
	typedef boost::shared_ptr<proxy::Connection> ConnectionPtr;

	namespace rpc {

		class Dispatcher;
		
		class RequestHandler {
		public:
			RequestHandler(Dispatcher &epollcore, ConnManager &front, ConnManager &back, int32 n = 4)
				:_epollcore(epollcore), _connmanager_front(front), _connmanager_back(back), _th_count(n) {
			}

			~RequestHandler() {
				for (std::vector<ThreadPtr>::iterator it = _threads.begin();
					it != _threads.end(); ++it) {
					if (*it) {
						(*it)->cancel();
						(*it)->join();
					}
				}
			}

			void dispatcher(const request &req);

			void back_reply_to_front(const request &req);//将后端服务器响应包转发给前端

			void run() {
				for (int16 i = 0; i < _th_count; ++i) {
					ThreadPtr ptr(new safe::Thread(boost::bind(&RequestHandler::worker, this)));
					_threads.push_back(ptr);
					ptr->start();
				}
			}
		private:
			void create_new_back_process(const request &req);

			void handle_toback_request(request &req);//处理响应到后端的请求
			void handle_ipc_request(const request &req);
			void handle_rpc_request(const request &req);

			void handle_inter_request(const request &req);

			void handle_error_request(const request &req, int32 err, const std::string &msg);
			void handle_unknow_request(const request &req);
			void handle_illegal_request(const request &req);


			void front_request_to_back(ConnectionPtr backptr, const request &req, const std::string &userid = "");//将前端请求包转发到后端服务器

																												  //handle request
			void worker() {
				LOG << "request handler worker running...";
				for (;;) {
					request req;
					_tasks.pop(req);
					handle_toback_request(req);
				}
			}


			Dispatcher &_epollcore;
			ConnManager &_connmanager_front;	//前端连接管理器
			ConnManager &_connmanager_back;		//后端连接管理器
			std::vector<ThreadPtr> _threads;
			int32 _th_count;

			safe::block_queue<request> _tasks;

			DISALLOW_COPY_AND_ASSIGN(RequestHandler);
		};

		typedef boost::shared_ptr<RequestHandler> RequestHandlerPtr;

	}//namespace rpc
}//namespace proxy


#endif