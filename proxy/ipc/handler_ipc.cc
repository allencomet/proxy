#include "handler_ipc.h"

#include "../core/connection.h"
#include "../core/connmanager.h"
#include "dispatcher_ipc.h"

namespace proxy {
	namespace ipc {

		void RequestHandler::dispatcher(const request &req) {
			switch (req.type) {
			case kIpcPack:
				_tasks.push(req);
				break;
			default:
				_epollcore.close_client(req.fd);
				break;
			}

		}

		//NOTE: 多个线程同时在执行此处,_connmanager取出的连接对象在epoll线程中会销毁，
		//但是因为使用了智能指针，所以在这里如果处理过程中连接被销毁了，那么在发送响
		//应包时会感知
		void RequestHandler::handle_request(request &req) {
			//将响应数据写入到这条连接的响应缓冲区里
			ConnectionPtr connptr = _connmanager.connection(req.fd);
			if (!connptr) return;

			if (BACK_EXIT == req.ipc_pack.msg_type){
				::FLG_stop_server = true;//setting the flag instruct server stop running
				WLOG << "received a message,server stop running immediately";
			} else {
				StreamBufPtr replyptr(new StreamBuf);
				LOG << "received client[" << connptr->addr() << "] request information: function_number[" << req.ipc_pack.func_no <<
					"],flow_no[" << req.ipc_pack.flow_no << "],msg_type[" << req.ipc_pack.msg_type << "],content[" << req.content.to_string() << "]";
				switch (req.ipc_pack.func_no) {
				case 1000:
				{
					std::string content = "handle 1000 function successful";
					req.ipc_pack.body_len = content.size();
					replyptr->append(&req.ipc_pack, g_kIPCLen);
					replyptr->append(content.c_str(), content.size());
				}
				break;
				case 2000:
				{
					std::string content = "handle 2000 function successful";
					req.ipc_pack.body_len = content.size();
					replyptr->append(&req.ipc_pack, g_kIPCLen);
					replyptr->append(content.c_str(), content.size());
				}
				break;
				default:
					break;
				}

				//将响应包放进响应队列里
				connptr->append_write_pack_s(replyptr);

				//将写缓冲区数据全部放在epoll中去操作
				_epollcore.register_mod_rw_event(req.fd);
			}
		}

	}//namespace ipc
}//namespace proxy