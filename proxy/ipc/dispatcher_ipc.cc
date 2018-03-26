#include "dispatcher_ipc.h"

#include "../core/parser.h"
#include "handler_ipc.h"


namespace proxy {
	namespace ipc {

		int32 Dispatcher::init_ipc_socket(const std::string &path) {
			unlink(path.c_str());
			net::unix_addr addr(path);
			int32 listenfd = net::unix_socket();
			net::setnonblocking(listenfd);

			if (listenfd < 0) {
				FLOG << "crate unix socket error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
			}

			if (!net::bind(listenfd, addr)) {
				return -1;
			}

			if (!net::listen(listenfd)) {
				return -1;
			}

			return listenfd;
		}

		void Dispatcher::init(int32 listenfd) {
			_listenfd = listenfd;

			_epfd = ::epoll_create(_event_limit);
			if (_epfd == -1) {
				FLOG << "epoll_create error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
			}

			_ev.events = EPOLLIN;
			_ev.data.fd = _listenfd;
			if (::epoll_ctl(_epfd, EPOLL_CTL_ADD, _listenfd, &_ev) == -1) {
				FLOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
			}

			_request_handler.reset(new RequestHandler(*this, _connmanager));
			_request_handler->run();
		}

		//Linux2.6版本之前还存在对于socket的accept的惊群现象,之后的版本已经解决掉了这个问题
		//惊群是指多个进程/线程在等待同一资源时，每当资源可用，所有的进程/线程都来竞争资源的现象
		void Dispatcher::init() {
			_listenfd = Dispatcher::init_ipc_socket(_path);

			_epfd = ::epoll_create(_event_limit);
			if (_epfd == -1) {
				FLOG << "epoll_create error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
			}

			_ev.events = EPOLLIN;
			_ev.data.fd = _listenfd;
			if (::epoll_ctl(_epfd, EPOLL_CTL_ADD, _listenfd, &_ev) == -1) {
				FLOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
			}

			_request_handler.reset(new RequestHandler(*this, _connmanager));
			_request_handler->run();
		}

		bool Dispatcher::handle_request(ConnectionPtr ptr) {
			bool status = true;
			for (;;) {
				request req;
				req.fd = ptr->fd();
				bool flag = false;
				switch (PaserTool::paser(ptr->rbuf(), req)) {
				case PaserTool::kPaserError:
					close_client(ptr->fd());//从epoll中移除该描述符(标记为等待关闭的连接)
					flag = true;//解析请求包出错，跳出for循环
					status = false;//表示出错，外层不必继续往下处理
					ELOG << "parser error";
					break;
				case PaserTool::kPaserInprogress:
					flag = true;//缓冲区内数据不足一个包，跳出for循环继续接收数据
					break;
				case PaserTool::kPaserHasdone:
					_request_handler->dispatcher(req);//将请求包放到请求队列
					flag = true;//已经处理完所有请求，跳出for循环继续接收数据
					break;
				case PaserTool::kPaserHasleft:
					_request_handler->dispatcher(req);//将请求包放到请求队列
					break;
				default://unknow request,close connection
					close_client(ptr->fd());//从epoll中移除该描述符(标记为等待关闭的连接)
					break;
				}

				if (flag) break;//跳出for循环
			}
			return status;
		}

		void Dispatcher::handle_accept_event(int32 fd) {
			//用while循环抱住accept调用，处理完TCP就绪队列中的所有连接后再退出循环,
			//accept返回-1并且errno设置为EAGAIN就表示所有连接都处理完
			int32 conn_sock;
			socklen_t clilen;
			struct sockaddr_un cliaddr;
			while ((conn_sock = ::accept(fd, (struct sockaddr *)&cliaddr, &clilen)) > 0) { //接受客户连接并返回已连接套接字描述符
				net::setnonblocking(conn_sock);  //设置套接字描述符为非阻塞的
				init_socket_option(conn_sock);	//设置套接字选项
				if (!_connmanager.add_conn(conn_sock, std::string(cliaddr.sun_path))) {//新建客户
					server_busy(conn_sock);
					return;
				} else {
					LOG << "accept new client: fd[" << conn_sock << "],addr[" << std::string(cliaddr.sun_path) << "]";
					register_add_read_event(conn_sock);
				}
			}

			if (conn_sock == -1) {
				if (errno != EAGAIN && errno != ECONNABORTED
					&& errno != EPROTO && errno != EINTR) {
					ELOG << "accept error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
				} else {
					LOG << "has accepted all clients";
				}
			}
		}

		void Dispatcher::handle_read_event(int32 fd) {
			//边缘触发模式下事件只会被触发一次，所以需要一次性把所有数据读取完
			int32 nread, n = 0;

			while ((nread = ::read(fd, _read_tmp_buf + n, BUFSIZ - 1)) > 0) {
				n += nread;
			}

			if (nread == -1 && errno != EAGAIN) {
				ELOG << "read error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
				close_client(fd);
			} else if (nread == 0) {
				WLOG << "closed by peer(" << fd << "): [" << errno << ":" << ::strerror(errno) << "]";
				close_client(fd);
			} else {
				ConnectionPtr connptr = _connmanager.connection(fd);
				if (connptr) {
					//LOG << "read " << n << " bytes data from client(" << fd << ":" << connptr->fd() << ")";
					connptr->set_msg_time(sys::utc.ms());
					connptr->append_rbuf_ns(_read_tmp_buf, n);
					handle_request(connptr);//处理请求包成功之后原来读事件上新增写事件（异步处理，在有数据响应的时候增加写事件）
				} else {
					close_client(fd);
				}
			}
		}

		void Dispatcher::handle_write_event(int32 fd) {
			ConnectionPtr connptr = _connmanager.connection(fd);
			if (connptr) {
				std::deque<StreamBufPtr> reply_queue;
				connptr->get_write_pack_s(reply_queue);//从用户响应队列中取出
				for (std::deque<StreamBufPtr>::iterator it = reply_queue.begin();
					it != reply_queue.end(); ++it) {
					if (-1 == net::writen(fd, (*it)->data(), (*it)->size())) {
						close_client(fd);//从epoll中移除该描述符
						break;
					}
				}
			} else {
				ELOG << "can't find the specify client(" << fd << ")";
				close_client(fd);
			}
		}

		void Dispatcher::close_client(int32 fd) {
			_ev.data.fd = fd;
			if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, &_ev) == -1) {
				ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
			}
			::close(fd);
			_connmanager.mark_death(fd);//标记为等待关闭的连接
		}

		void Dispatcher::server_busy(int32 fd) {
			WLOG << "server busy...";
			close_client(fd);
		}

		void Dispatcher::init_socket_option(int32 fd) {
			int nRecvBuf = 128 * 1024;//设置为32K  
			::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));
			//发送缓冲区  
			int32 nSendBuf = 128 * 1024;//设置为32K  
			::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBuf, sizeof(int));

			int32 nNetTimeout = 1000;//1秒  
									 //发送时限  
			::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&nNetTimeout, sizeof(int));

			//接收时限  
			::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&nNetTimeout, sizeof(int));
		}

		void Dispatcher::run() {
			LOG << "server launch...";
			int nfds, fd, i;
			for (;;) {
				//在这里负责连接的删除
				_connmanager.check_invalid_conn();

				//等待描述符准备就绪，返回就绪数量
				int32 nfds = ::epoll_wait(_epfd, _events, MAX_EVENTS, 100);
				if (nfds == -1) {
					ELOG << "epoll_wait error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
					continue;
				}

				for (i = 0; i < nfds; ++i) {
					fd = _events[i].data.fd;
					if (fd == _listenfd) {
						handle_accept_event(fd);
					} else if (_events[i].events & EPOLLIN) {
						handle_read_event(fd);
					} else if (_events[i].events & EPOLLOUT) {
						handle_write_event(fd);
					} else if (_events[i].events & EPOLLHUP) {
						ELOG << "exception occurred(" << fd << "): [" << errno << ":" << ::strerror(errno) << "]";
						close_client(fd);
					}
				}//end for
			}//end for(;;)
		}


	}//namespace ipc
}//namespace proxy