#ifndef __PROXY_RPC_DIPATCHER_H
#define __PROXY_RPC_DIPATCHER_H

#include "../../util/util.h"
#include "../core/protocol.h"
#include "../core/connection.h"
#include "../core/connmanager.h"
#include "../core/connmap.h"
#include "handler_rpc.h"

#define MAX_EVENTS 10

namespace proxy {
	namespace rpc {

		class Dispatcher {
		public:
			//RPC server
			explicit Dispatcher(const std::string &ip, const int16 &port)
				:_ip(ip),
				_port(port),
				_listenfd(0),
				_epfd_front(0),
				_event_limit(10),
				_listen_limit(20) {
				_connmapptr.reset(new Connmap(_connmanager_front, _connmanager_back));
			}

			~Dispatcher() {
				inform_back_exit();

				if (_front_threadptr) {
					_front_threadptr->cancel();
					_front_threadptr->join();
				}
				if (_back_threadptr) {
					_back_threadptr->cancel();
					_back_threadptr->join();
				}
			}

			static int32 init_tcp_socket(const std::string &ip, const int16 &port);

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
			void run_front();
			void run_back();

			bool handle_request_from_front(ConnectionPtr ptr);
			bool handle_reply_from_back(ConnectionPtr ptr);

			void handle_accept_event(int32 fd);

			void handle_read_event_front(int32 fd);
			void handle_read_event_back(int32 fd);

			void handle_write_event_front(int32 fd);
			void handle_write_event_back(int32 fd);

			void close_client_front(int32 fd);
			void close_client_back(int32 fd);

			void server_busy(int32 fd);

			void init_socket_option(int32 fd);

			void inform_back_exit();

			inline bool register_add_read_event_front(int32 fd) {
				safe::MutexGuard g(_event_mtx_front);
				_ev_front.events = EPOLLIN | EPOLLET;  //边缘触发模式
				_ev_front.data.fd = fd;
				if (epoll_ctl(_epfd_front, EPOLL_CTL_ADD, fd, &_ev_front) == -1) {
					FLOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
					return false;
				}
				return true;
			}

			inline bool register_add_read_event_back(int32 fd) {
				safe::MutexGuard g(_event_mtx_back);
				_ev_back.events = EPOLLIN | EPOLLET;  //边缘触发模式
				_ev_back.data.fd = fd;
				if (epoll_ctl(_epfd_back, EPOLL_CTL_ADD, fd, &_ev_back) == -1) {
					FLOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
					return false;
				}
				return true;
			}

			inline bool register_mod_read_event_front(int32 fd) {
				safe::MutexGuard g(_event_mtx_front);
				_ev_front.data.fd = fd;
				_ev_front.events = EPOLLIN | EPOLLET;
				if (epoll_ctl(_epfd_front, EPOLL_CTL_MOD, fd, &_ev_front) == -1) {
					ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
					return false;
				}
				return true;
			}

			inline bool register_mod_write_event_front(int32 fd) {
				safe::MutexGuard g(_event_mtx_front);
				_ev_front.data.fd = fd;
				_ev_front.events = EPOLLOUT | EPOLLET;
				if (epoll_ctl(_epfd_front, EPOLL_CTL_MOD, fd, &_ev_front) == -1) {
					ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
					return false;
				}
				return true;
			}

			inline bool register_mod_rw_event_front(int32 fd) {
				safe::MutexGuard g(_event_mtx_front);
				_ev_front.data.fd = fd;
				_ev_front.events = EPOLLIN | EPOLLOUT | EPOLLET;
				if (epoll_ctl(_epfd_front, EPOLL_CTL_MOD, fd, &_ev_front) == -1) {
					ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
					return false;
				}
				return true;
			}

			inline bool register_mod_rw_event_back(int32 fd) {
				safe::MutexGuard g(_event_mtx_back);
				_ev_back.data.fd = fd;
				_ev_back.events = EPOLLIN | EPOLLOUT | EPOLLET;
				if (epoll_ctl(_epfd_back, EPOLL_CTL_MOD, fd, &_ev_back) == -1) {
					ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
					return false;
				}
				return true;
			}

			std::string _ip;//server ip
			int16 _port;//server port
			int32 _listenfd;
			int32 _epfd_front;
			int32 _epfd_back;
			int32 _listen_limit;
			int32 _event_limit;
			char _read_tmp_buf_front[MAXLINE];
			char _read_tmp_buf_back[MAXLINE];

			RequestHandlerPtr _request_handler;//处理请求句柄

			ConnManager _connmanager_front;	//前端连接管理器
			ConnManager _connmanager_back;	//后端连接管理器
			ConnmapPtr _connmapptr;			//前后端映射管理器

			ThreadPtr _back_threadptr;
			ThreadPtr _front_threadptr;

			safe::Mutex _event_mtx_front;//epoll非线程安全的，不能在多个线程去对事件增删改
			struct epoll_event _ev_front;
			struct epoll_event _events_front[MAX_EVENTS];

			safe::Mutex _event_mtx_back;//epoll非线程安全的，不能在多个线程去对事件增删改
			struct epoll_event _ev_back;
			struct epoll_event _events_back[MAX_EVENTS];

			DISALLOW_COPY_AND_ASSIGN(Dispatcher);
		};

	}//namespace rpc
}//namespace proxy

#endif