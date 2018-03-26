#ifndef __PROXY_IPC_DISPATCHER_H
#define __PROXY_IPC_DISPATCHER_H

#include "../../util/util.h"
#include "../core/protocol.h"
#include "../core/connection.h"
#include "../core/connmanager.h"


#define MAX_EVENTS 10

namespace proxy {
	namespace ipc {

		class RequestHandler;
		typedef boost::shared_ptr<RequestHandler> RequestHandlerPtr;

		class Dispatcher {
		public:
			//IPC server
			explicit Dispatcher(const std::string &path)
				:_path(path),
				_listenfd(0),
				_epfd(0),
				_event_limit(10),
				_listen_limit(20) {

			}

			static int32 init_ipc_socket(const std::string &path);

			inline void set_listen_limit(int32 n) {
				_listen_limit = n;
			}

			inline int32 listen_limit() const {
				return _listen_limit;
			}

			inline void set_event_limit(int32 n) {
				_listen_limit = n;
			}

			inline int32 event_limit() const {
				return _event_limit;
			}
			void init(int32 listenfd);
			void init();

			void run();

			friend class RequestHandler;//声明request_handler为友元类，允许访问私有成员
		private:
			bool handle_request(ConnectionPtr ptr);

			void handle_accept_event(int32 fd);

			void handle_read_event(int32 fd);

			void handle_write_event(int32 fd);

			void close_client(int32 fd);

			void server_busy(int32 fd);

			void init_socket_option(int32 fd);

			inline bool register_add_read_event(int32 fd) {
				safe::MutexGuard g(_event_mtx);
				_ev.events = EPOLLIN | EPOLLET;  //边缘触发模式
				_ev.data.fd = fd;
				if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &_ev) == -1) {
					FLOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
					return false;
				}
				return true;
			}

			inline bool register_mod_read_event(int32 fd) {
				safe::MutexGuard g(_event_mtx);
				_ev.data.fd = fd;
				_ev.events = EPOLLIN | EPOLLET;
				if (epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &_ev) == -1) {
					ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
					return false;
				}
				return true;
			}

			inline bool register_mod_write_event(int32 fd) {
				safe::MutexGuard g(_event_mtx);
				_ev.data.fd = fd;
				_ev.events = EPOLLOUT | EPOLLET;
				if (epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &_ev) == -1) {
					ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
					return false;
				}
				return true;
			}

			inline bool register_mod_rw_event(int32 fd) {
				safe::MutexGuard g(_event_mtx);
				_ev.data.fd = fd;
				_ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
				if (epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &_ev) == -1) {
					ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
					return false;
				}
				return true;
			}

			std::string _path;
			int32 _listenfd;
			int32 _epfd;
			int32 _listen_limit;
			int32 _event_limit;
			char _read_tmp_buf[4096];

			ConnManager _connmanager;//连接管理器
			RequestHandlerPtr _request_handler;//处理请求句柄

			safe::Mutex _event_mtx;//epoll非线程安全的，不能在多个线程去对事件增删改
			struct epoll_event _ev;
			struct epoll_event _events[MAX_EVENTS];

			DISALLOW_COPY_AND_ASSIGN(Dispatcher);
		};

	}//namespace ipc
}//namespace proxy

#endif