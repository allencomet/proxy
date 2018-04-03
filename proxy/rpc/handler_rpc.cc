#include "handler_rpc.h"

#include "../core/connection.h"
#include "../core/connmanager.h"
#include "dispatcher_rpc.h"

namespace {
	int32 connect_back_server(const std::string &path) {
		int32 connfd;
		if ((connfd = net::unix_socket()) < 0) {
			LOG << "create unix socket error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
			return -1;
		}
		net::setnonblocking(connfd);
		net::unix_addr remoteaddr(path);
		if (net::connect(connfd, remoteaddr)){
			return connfd;
		}
		::close(connfd);
		return -1;
	}

	const std::string kCurrentAbPath = os::get_current_dir_name();
}


namespace proxy {
	namespace rpc {
		const int32 kProxyInterface = 1000;
		const int32 kRPCInterface = 2000;
		const int32 kIPCInterface = 3000;

		const ErrMsg g_kUnkowErrMsg(-1, "unknow request");
		const ErrMsg g_kIllegalErrMsg(-2, "illegal request");
		const ErrMsg g_kNotFindErrMsg(-3, "input param invalid,can't find user id");

		//NOTE: 多个线程同时在执行此处,_connmanager取出的连接对象在epoll线程中会销毁，
		//但是因为使用了智能指针，所以在这里如果处理过程中连接被销毁了，那么在发送响
		//应包时会感知
		void RequestHandler::handle_inter_request(const request &req) {
			//将响应数据写入到这条连接的响应缓冲区里
			ConnectionPtr connptr = _connmanager_front.connection(req.fd);
			if (!connptr) return;

			LOG << "received client[" << connptr->addr() << "]: function_number[" << req.rpc_pack.func_no << "],flow_no["
				<< req.rpc_pack.flow_no << "],msg_type[" << req.rpc_pack.msg_type << "],content[" << req.content.data() << "]";

			StreamBuf content;
			content << "{\"errno\":0,\"errmsg\":\"successful\",\"result\":\"handle rpc function successful\"}";
			StreamBufPtr replyptr(new StreamBuf);
			RPCPACK pack;
			::memset(&pack, 0, g_kRPCLen);
			::memcpy(&pack, &req.rpc_pack, g_kRPCLen);
			pack.body_len = content.size();
			replyptr->append((char *)&pack, g_kRPCLen);
			replyptr->append(content.data(), content.size());

			//将响应包放进响应队列里
			connptr->append_write_pack_s(replyptr);
			//将写缓冲区数据全部放在epoll中去操作
			_epollcore.register_mod_rw_event_front(req.fd);
		}

		void RequestHandler::create_new_back_process(const request &req) {
			std::string userid = "";
			Json::Reader jsonReader;
			Json::Value jsonValue;

			if (jsonReader.parse(req.content.data(), jsonValue)) userid = jsonValue["user_id"].asString();//从json串中解析出用户ID
			if (userid.empty()) {
				ELOG << "input param invalid,can't find user id";
				handle_error_request(req, g_kNotFindErrMsg.err_no, g_kNotFindErrMsg.err_msg);
				return;
			}

			std::string srvpath = kCurrentAbPath + "/bus/unixsock/" + userid;
			std::string srvcmd = kCurrentAbPath + "/bus/backend -srvpath=" + srvpath + " -log_dir=" +
				kCurrentAbPath + "/bus/" + userid + "_log &";
			int32 connfd = connect_back_server(srvpath);
			if (-1 == connfd) {
				//(1)system指定监听路径方式启动后端进程
				os::system(srvcmd);
				save_back_path(srvpath);
				//sys::msleep(100);//等待后端进程进行一些基本的初始化操作
				//(2)根据指定路径连接后端进程
				connfd = connect_back_server(srvpath);
				if (connfd == -1) {
					handle_illegal_request(req);
					return;
				}
				LOG << "create a new backend[" << userid << "]: " << srvpath;
			} else {
				LOG << "there's a backend[" << userid << "] exist: " << srvpath;
			}

			ConnectionPtr backptr(new Connection(connfd, srvpath));
			//(3)需要将新的连接添加到后端连接管理器以及前后端映射的map里
			_connmanager_back.add_conn(connfd, backptr);
			_epollcore._connmapptr->associate(req.fd, connfd);
			//(4)将该连接注册到后端监听读事件
			_epollcore.register_add_read_event_back(connfd);
			//(5)将请求发送给后端进程
			front_request_to_back(backptr, req);
		}

		void RequestHandler::handle_ipc_request(const request &req) {
			back_reply_to_front(req);//将后端响应包转发至前端
		}

		void RequestHandler::handle_rpc_request(const request &req) {
			//判断这个请求对应的后端连接是否存在，如果不存在且请求接口为登录接口则创建，否则报未登录
			switch (req.rpc_pack.func_no) {
			case kProxyInterface:
				if (req.rpc_pack.func_no == kProxyInterface) {
					ConnectionPtr backptr = _epollcore._connmapptr->back_connptr(req.fd);
					if (!backptr) {
						create_new_back_process(req);//create an back end(just first time)
					} else {
						_tasks.push(req);
					}
				} else {
					ConnectionPtr backptr = _epollcore._connmapptr->back_connptr(req.fd);
					if (!backptr) {
						handle_illegal_request(req);
					} else {
						_tasks.push(req);
					}
				}
				break;
			case kRPCInterface:
				_tasks.push(req);//模拟一个耗时的操作，放到任务池处理，直接从代理服务器响应
				break;
			default:
				_tasks.push(req);//其他请求均往后抛
				break;
			}

		}

		void RequestHandler::handle_error_request(const request &req, int32 err, const std::string &msg) {
			ConnectionPtr connptr = _connmanager_front.connection(req.fd);
			if (!connptr) return;
			StreamBufPtr replyptr(new StreamBuf);
			LOG << "received client[" << connptr->addr() << "]: function_number[" << req.rpc_pack.func_no << "],flow_no["
				<< req.rpc_pack.flow_no << "],msg_type[" << req.rpc_pack.msg_type << "],content[" << req.content.data() << "]";
			StreamBuf content;
			content << "{\"errno\": " << err << " ,\"errmsg\":\"" << msg << "\",\"result\":\"\"}";
			replyptr->append((char *)&req.rpc_pack, g_kRPCLen);
			RPCPACK *tmp_pack = (RPCPACK *)replyptr->data();
			tmp_pack->body_len = content.size();
			replyptr->append(content.data(), content.size());
			//将响应包放进响应队列里
			connptr->append_write_pack_s(replyptr);
			//将写缓冲区数据全部放在epoll中去操作
			_epollcore.register_mod_rw_event_front(req.fd);
		}

		void RequestHandler::handle_unknow_request(const request &req) {
			handle_error_request(req, g_kUnkowErrMsg.err_no, g_kUnkowErrMsg.err_msg);
		}

		void RequestHandler::handle_illegal_request(const request &req) {
			handle_error_request(req, g_kIllegalErrMsg.err_no, g_kIllegalErrMsg.err_msg);
		}


		void RequestHandler::dispatcher(const request &req) {
			switch (req.type) {
			case kIpcPack:
				handle_ipc_request(req);//IPC包
				break;
			case kRpcPack:
				handle_rpc_request(req);//RPC包
				break;
			default:
				handle_unknow_request(req);
				break;
			}
		}

		void RequestHandler::back_reply_to_front(const request &req) {
			ConnectionPtr connptr = _epollcore._connmapptr->front_connptr(req.fd);
			if (!connptr) return;
			LOG << "received back[" << connptr->addr() << "]: function_number[" << req.ipc_pack.func_no << "],flow_no["
				<< req.ipc_pack.flow_no << "],msg_type[" << req.ipc_pack.msg_type << "],content[" << req.content.data() << "]";
			StreamBufPtr replyptr(new StreamBuf);

			RPCPACK pack;
			::memcpy(pack.tag, g_kRPCTag.c_str(), g_kRPCTag.size());
			pack.flow_no = req.ipc_pack.flow_no;
			pack.func_no = req.ipc_pack.func_no;
			pack.body_len = req.content.size();

			replyptr->append(&pack, g_kRPCLen);
			replyptr->append(req.content.data(), req.content.size());

			//将响应包放进响应队列里
			connptr->append_write_pack_s(replyptr);
			//将写缓冲区数据全部放在epoll中去操作
			_epollcore.register_mod_rw_event_front(connptr->fd());
		}

		//处理响应到后端的请求
		void RequestHandler::handle_toback_request(request &req) {
			ConnectionPtr connptr = _connmanager_front.connection(req.fd);
			if (!connptr) return;

			switch (req.rpc_pack.func_no) {
			case kRPCInterface:
				handle_inter_request(req);
				break;
			default:
			{
				ConnectionPtr backptr = _epollcore._connmapptr->back_connptr(req.fd);
				if (!backptr) {
					create_new_back_process(req);//创建新的后端进程,并且创建到后端进程的连接
				} else {
					front_request_to_back(backptr, req);//将请求发送给后端进程
				}
			}
			break;
			}
		}

		//将前端请求包转发到后端服务器
		void RequestHandler::front_request_to_back(ConnectionPtr backptr, const request &req, const std::string &userid) {
			StreamBufPtr bufptr(new StreamBuf);

			//转换成IPC协议与后端服务进程通信
			IPCPACK pack;
			::memset(&pack, 0, g_kIPCLen);
			::memcpy(pack.tag, g_kIPCTag.c_str(), g_kIPCTag.size());
			::memcpy(pack.ueser_id, userid.c_str(), userid.size());
			pack.msg_type = BACK_MSG;
			pack.flow_no = req.rpc_pack.flow_no;
			pack.func_no = req.rpc_pack.func_no;
			pack.body_len = req.content.size();
			bufptr->append(&pack, g_kIPCLen);//IPC protocol
			bufptr->append(req.content.data(), req.content.size());

			//为了确保不在多个处理线程内同时往这个后端套接字发送数据，将请求包放到请求队列，在epoll中发送
			backptr->append_write_pack_s(bufptr);
			//将写缓冲区数据全部放在epoll中去操作
			_epollcore.register_mod_rw_event_back(backptr->fd());
		}

		//将包广播给所有后端服务器进程
		void RequestHandler::inform_back_exit() {
			IPCPACK pack;
			::memset(&pack, 0, g_kIPCLen);
			::memcpy(pack.tag, g_kIPCTag.c_str(), g_kIPCTag.size());
			pack.msg_type = BACK_EXIT;

			//如果客户连接已经关闭了，在这里连接也取不到，所以无法通知后端进程退出，只能在创建后端进程时保存下已经创建的后端进程的地址
			//然后再这里去通知后端进程退出
			//WLOG << "inform backend exit,there are [" << _connmanager_back.size() << "] connection in pool";
			//ConnManager::CMIterator iter(_connmanager_back);
			//for (ConnectionPtr it = iter.begin(); !iter.end(); it = iter.next()) {
			//	if (it) {
			//		WLOG << "inform backend[" << it->addr() << "] to exit...";
			//		StreamBufPtr bufptr(new StreamBuf);
			//		bufptr->append(&pack, g_kIPCLen);
			//		//为了确保不在多个处理线程内同时往这个后端套接字发送数据，将请求包放到请求队列，在epoll中发送
			//		it->append_write_pack_s(bufptr);
			//		//将写缓冲区数据全部放在epoll中去操作
			//		_epollcore.register_mod_rw_event_back(it->fd());
			//	}
			//}

			WLOG << "prepare send message to all backends";
			safe::MutexGuard g(_back_path_mtx);//后端服务器进程的地址
			for (std::set<std::string>::iterator it = _back_path.begin(); it != _back_path.end(); ++it) {
				WLOG << "prepare send message to back[" << *it << "]";
				aux_inform_to_back(*it, pack);//通过广播通知子进程关闭
				std::vector<int32> pids = os::check_process(*it);
				for (std::vector<int32>::iterator itp = pids.begin(); itp != pids.end(); ++itp){
					::kill(*itp, SIGKILL);//如果子进程没有关闭，强制关闭后端服务进程
					WLOG << "kill [path:" << *it << "][pid:" << *itp << "]...";
				}
			}
		}

		bool RequestHandler::aux_inform_to_back(const std::string &srvpath, IPCPACK &pack) {
			bool flag = false;
			int32 connfd = connect_back_server(srvpath);
			if (-1 != connfd) {
				if (-1 == net::writen(connfd, &pack, g_kIPCLen)) {
					WLOG << "failed to inform back[" << srvpath << "] exit...";
				} else {
					WLOG << "inform back[" << srvpath << "] exit...";
					flag = true;
				}
				::close(connfd);
			}
			return flag;
		}

	}//namespace rpc
}//namespace proxy