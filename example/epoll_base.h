#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "../util/util.h"

#define MAX_EVENTS 10
#define PORT 8080

namespace epollbase {
#pragma pack(push, 1)
	typedef struct IPCPACK {
		char	_tag[5];		//TKIPC
		char	_ueser_id[16];	//用户ID
		int32	_msg_type;		//消息类型(0:进程退出,1:业务消息,2:主进程检测到交易日更新)
		int32	_flow_no;		//流水号
		uint32	_func_no;		//功能号
		int32	_errorno;		//错误码(请求包设置0)
		uint32	_bodylen;		//数据长度
	}IPCPACK;
#pragma pack(pop)

	typedef struct request {
		int32 _fd;
		IPCPACK _pack;
		std::string _content;
	}request;

	static const std::string PACKTAG = "TKIPC";
	static const int32 IPCPACKLEN = sizeof(IPCPACK);

	//设置socket连接为非阻塞模式
	void setnonblocking(int sockfd) {
		int opts;
		opts = ::fcntl(sockfd, F_GETFL);
		if (opts < 0) {
			::perror("fcntl(F_GETFL)\n");
			::exit(1);
		}
		opts = (opts | O_NONBLOCK);
		if (::fcntl(sockfd, F_SETFL, opts) < 0) {
			::perror("fcntl(F_SETFL)\n");
			::exit(1);
		}
	}

	class connection {
	public:
		connection(int32 fd) :_fd(fd) {
		}
		connection(int32 fd, const std::string &addr) :_fd(fd), _complete_addr(addr) {
		}
		~connection() {}


		inline int32 fd() const {
			return _fd;
		}

		std::string addr() const {
			return _complete_addr;
		}

		void set_addr(const std::string &addr) {
			_complete_addr = addr;
		}

		StreamBuf &rbuf() {
			return _rbuf;
		}

		StreamBuf &wbuf() {
			return _wbuf;
		}

		void safe_append_rbuf(const char *data, int32 n) {
			safe::MutexGuard g(_rmutex);
			_rbuf.append(data, n);
		}

		void safe_append_wbuf(const char *data, int32 n) {
			safe::MutexGuard g(_wmutex);
			_wbuf.append(data, n);
		}

		void unsafe_append_rbuf(const char *data, int32 n) {
			_rbuf.append(data, n);
		}

		void unsafe_append_wbuf(const char *data, int32 n) {
			_wbuf.append(data, n);
		}

		inline void set_msg_time(int64 t) {
			_time = t;
		}

		inline int64 msg_time() const {
			return _time;
		}
	private:
		int32 _fd;
		int64 _time;//last msg time
		std::string _complete_addr;


		safe::Mutex _rmutex;
		StreamBuf _rbuf;

		safe::Mutex _wmutex;
		StreamBuf _wbuf;

		DISALLOW_COPY_AND_ASSIGN(connection);
	};

	typedef boost::shared_ptr<connection> conncoreptr;

	class connmanager {
	public:
		connmanager():_idle_time(60), _max_conn_limit(5000){}
		~connmanager() {}

		int64 idle_time() const {
			return _idle_time;
		}

		void set_idle_time(int64 n) {
			_idle_time = n;
		}

		int32 max_conn_limit() const {
			return _max_conn_limit;
		}

		void set_max_conn_limit(int32 n) {
			_max_conn_limit = n;
		}

		bool add_conn(int32 fd, conncoreptr ptr) {
			if (_conns.size() >= _max_conn_limit) {
				return false;
			}
			_conns.insert(std::make_pair(fd, ptr));
			return true;
		}

		bool add_conn(int32 fd, const std::string &addr) {
			if (_conns.size() >= _max_conn_limit) {
				return false;
			}
			conncoreptr ptr(new epollbase::connection(fd, addr));
			ptr->set_msg_time(sys::utc.ms());
			_conns.insert(std::make_pair(fd, ptr));
			return true;
		}

		void mark_death(int32 fd) {
			_kill_fds.insert(fd);
		}

		void remove_conn(int32 fd) {
			boost::unordered_map<int32, conncoreptr>::iterator it = _conns.find(fd);
			if (it != _conns.end()){
				WLOG << "close client: fd[" << it->first << "],addr[" << it->second->addr() << "]...";
				::close(fd);
				_conns.erase(it);
			}
		}

		void check_invalid_conn() {
			//WLOG << "checck invalid client";
			for (boost::unordered_map<int32, conncoreptr>::iterator it = _conns.begin();
				it != _conns.end();) {
				if (sys::utc.sec() - it->second->msg_time() > _idle_time) {
					WLOG << "check client idle timeout: fd[" << it->first << "],addr[" << it->second->addr() << "]...";
					::close(it->second->fd());
					_conns.erase(it++);
				}else {
					++it;
				}
			}

			for (boost::unordered_set<int32>::iterator it = _kill_fds.begin();
				it != _kill_fds.end(); ++it) {
				remove_conn(*it);
				_conns.erase(*it);
			}
			_kill_fds.clear();
		}

		conncoreptr connection(int32 fd) {
			conncoreptr ptr;
			boost::unordered_map<int32, conncoreptr>::iterator it = _conns.find(fd);
			if (it != _conns.end()) {
				ptr = it->second;
			}else {
				errorlog::err_msg("can't find fd[%d] client", fd);
			}
			return ptr;
		}
	private:
		boost::unordered_map<int32, conncoreptr> _conns;
		boost::unordered_set<int32> _kill_fds;
		int64 _idle_time;
		int32 _max_conn_limit;

		DISALLOW_COPY_AND_ASSIGN(connmanager);
	};

	class pasermanager {
	public:
		typedef enum PASERSTATUS {
			PASER_ERROR,
			PASER_INPROGRESS,
			PASER_HASDONE,
			PASER_HASLEFT
		}PASERSTATUS;

		static PASERSTATUS paser(StreamBuf &buf, request &req) {
			if (buf.size() < 5) {
				return PASER_INPROGRESS;
			}

			//协议不匹配
			if (0 != ::memcmp(buf.data(), PACKTAG.c_str(), 5)) {
				return PASER_ERROR;
			}

			if (buf.size() < IPCPACKLEN) {
				return PASER_INPROGRESS;
			}
			else {
				IPCPACK *pack = (IPCPACK *)buf.data();
				int32 expect_packlen = pack->_bodylen + IPCPACKLEN;
				if (expect_packlen < buf.size()) {//包未收完整
					return PASER_INPROGRESS;
				}
				else {
					::memcpy(&req._pack, buf.data(), IPCPACKLEN);
					req._content.assign(buf.data() + IPCPACKLEN, pack->_bodylen);
					buf.clear_head(expect_packlen);
					if (0 == buf.size()) {
						return PASER_HASDONE;
					}
					else {
						return PASER_HASLEFT;
					}
				}
			}
		}

	private:
		pasermanager();
		~pasermanager();

		DISALLOW_COPY_AND_ASSIGN(pasermanager);
	};

	

	typedef boost::shared_ptr<safe::Thread> ThreadPtr;
	typedef boost::shared_ptr<request> requestptr;


	class epollcore {
	public:
		epollcore(const std::string &ip, const int16 &port)
			:_ip(ip),
			_port(port),
			_listenfd(0),
			_epfd(0),
			_event_limit(0),
			_listen_limit(0) {

		}

		void set_listen_limit(int32 n) {
			_listen_limit = n;
		}

		int32 listen_limit() const {
			return _listen_limit;
		}

		void set_event_limit(int32 n) {
			_listen_limit = n;
		}

		int32 event_limit() const {
			return _event_limit;
		}

		void init() {
			_listenfd = net::tcp_socket();
			setnonblocking(_listenfd);
			net::ipv4_addr localaddr(_ip, _port);
			net::bind(_listenfd, localaddr);
			net::listen(_listenfd, _listen_limit);

			_epfd = ::epoll_create(MAX_EVENTS);
			if (_epfd == -1) {
				FLOG << "epoll_create error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
			}

			_ev.events = EPOLLIN;
			_ev.data.fd = _listenfd;
			if (::epoll_ctl(_epfd, EPOLL_CTL_ADD, _listenfd, &_ev) == -1) {
				FLOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
			}
		}

		void run() {
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
					}
				}//end for
			}//end for(;;)
		}
	private:
		void handle_reply(conncoreptr connptr,request &req) {
			LOG << "received client[" << connptr->addr() << "] request information: function_number[" << req._pack._func_no <<
				"],flow_no[" << req._pack._flow_no << "],msg_type[" << req._pack._msg_type << "],content[" << req._content << "]";
			switch (req._pack._func_no){
			case 10001: 
				{
					std::string content = "handle 10001 function successful";
					StreamBuf &buf = connptr->wbuf();
					req._pack._bodylen = content.size();
					buf.append(&req._pack, IPCPACKLEN);
					buf.append(content.c_str(),content.size());
				}
				break;
			case 10002:
				{
					std::string content = "handle 10002 function successful";
					StreamBuf &buf = connptr->wbuf();
					req._pack._bodylen = content.size();
					buf.append(&req._pack, IPCPACKLEN);
					buf.append(content.c_str(), content.size());
				}
				break;
			case 10003:
				{
					std::string content = "handle 10003 function successful";
					StreamBuf &buf = connptr->wbuf();
					req._pack._bodylen = content.size();
					buf.append(&req._pack, IPCPACKLEN);
					buf.append(content.c_str(), content.size());
				}
				break;
			default:
				break;
			}
		}

		bool handle_request(conncoreptr ptr) {
			bool status = true;
			for (;;){
				request req;
				req._fd = ptr->fd();
				bool flag = false;
				switch (pasermanager::paser(ptr->rbuf(), req)){
				case pasermanager::PASER_ERROR:
					close_client(ptr->fd());//标记为等待关闭的连接
					flag = true;//解析请求包出错，跳出for循环
					status = false;//表示出错，外层不必继续往下处理
					errorlog::err_msg("parser error");
					break;
				case pasermanager::PASER_INPROGRESS:
					flag = true;//缓冲区内数据不足一个包，跳出for循环继续接收数据
					break;
				case pasermanager::PASER_HASDONE:
					handle_reply(ptr,req);
					flag = true;//已经处理完所有请求，跳出for循环继续接收数据
					break;
				case pasermanager::PASER_HASLEFT:
					handle_reply(ptr, req);
					break;
				default://unknow request,close connection
					close_client(ptr->fd());//标记为等待关闭的连接
					break;
				}

				if (flag) break;//跳出for循环
			}
			return status;
		}

		void handle_accept_event(int32 fd) {
			//用while循环抱住accept调用，处理完TCP就绪队列中的所有连接后再退出循环,
			//accept返回-1并且errno设置为EAGAIN就表示所有连接都处理完。
			int32 conn_sock;
			net::ipv4_addr remoteaddr;

			while ((conn_sock = net::accept(fd, &remoteaddr)) > 0) { //接受客户连接并返回已连接套接字描述符
				setnonblocking(conn_sock);  //设置套接字描述符为非阻塞的
				if (!_connmanager.add_conn(conn_sock, remoteaddr.to_string())) {//新建客户
					server_busy(conn_sock);
					return;
				}else {
					LOG << "accept new client: fd[" << conn_sock << "],addr[" << remoteaddr.to_string() << "]";
					register_read_event(conn_sock);
				}
			}

			if (conn_sock == -1 && errno == EAGAIN) {
				if (errno != EAGAIN && errno != ECONNABORTED
					&& errno != EPROTO && errno != EINTR) {
					ELOG << "accept error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
				}
			}
		}

		void handle_read_event(int32 fd) {
			//边缘触发模式下事件只会被触发一次，所以需要一次性把所有数据读取完
			int32 n = net::readn(fd, _read_tmp_buf, BUFSIZ - 1);
			if (n > 0) {
				//LOG << "read " << n << " bytes data from client";
				conncoreptr connptr = _connmanager.connection(fd);
				if (connptr) {
					connptr->unsafe_append_rbuf(_read_tmp_buf, n);
					connptr->set_msg_time(sys::utc.ms());
					if (handle_request(connptr)) {
						register_rw_event(fd);//处理请求包成功之后原来读事件上新增写事件
					}
				}else {
					close_client(fd);
				}
			}else if (n == 0) {
				WLOG << "closed by peer: [" << errno << ":" << ::strerror(errno) << "]";
				close_client(fd);
			}else {
				ELOG << "read error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
				close_client(fd);
			}
		}

		void handle_write_event(int32 fd) {
			conncoreptr connptr = _connmanager.connection(fd);
			if (connptr) {
				StreamBuf &wbuf = connptr->wbuf();//将写缓冲区数据一次全部发送出去
				//边缘触发模式下事件只会被触发一次，所以需要一次性把所有数据发送完
				if (-1 == net::writen(fd, wbuf.data(), wbuf.size())) {
					close_client(fd);//从epoll中移除该描述符
				}
				wbuf.clear();//将写缓冲区数据发送完之后清空
			}else {
				close_client(fd);
			}
		}

		bool setnonblocking(int sockfd) {
			int opts;
			opts = ::fcntl(sockfd, F_GETFL);
			if (opts < 0) {
				return false;
			}
			opts = (opts | O_NONBLOCK);
			if (::fcntl(sockfd, F_SETFL, opts) < 0) {
				return false;
			}
			return true;
		}

		inline void close_client(int32 fd) {
			_ev.data.fd = fd;
			if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, &_ev) == -1) {
				ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
			}
			_connmanager.mark_death(fd);//标记为等待关闭的连接
			::close(fd);
		}

		inline void server_busy(int32 fd) {
			WLOG << "server busy...";
			close_client(fd);
		}

		inline bool register_read_event(int32 fd) {
			_ev.events = EPOLLIN | EPOLLET;  //边缘触发模式
			_ev.data.fd = fd;
			if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &_ev) == -1) {
				FLOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
				return false;
			}
			return true;
		}

		inline bool register_write_event(int32 fd) {
			_ev.data.fd = fd;
			_ev.events = EPOLLOUT | EPOLLET;
			if (epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &_ev) == -1) {
				ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
				return false;
			}
			return true;
		}

		inline bool register_rw_event(int32 fd) {
			_ev.data.fd = fd;
			_ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
			if (epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &_ev) == -1) {
				ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
				return false;
			}
			return true;
		}

		std::string _ip;//server ip
		int16 _port;//server port
		int32 _listenfd;
		int32 _epfd;
		int32 _listen_limit;
		int32 _event_limit;
		char _read_tmp_buf[4096];

		connmanager _connmanager;//连接管理器

		struct epoll_event _ev;
		struct epoll_event _events[MAX_EVENTS];
		DISALLOW_COPY_AND_ASSIGN(epollcore); 
	};
}



/*
在一个非阻塞的socket上调用read/write函数, 返回EAGAIN或者EWOULDBLOCK(注: EAGAIN就是EWOULDBLOCK)
从字面上看, 意思是:EAGAIN: 再试一次，EWOULDBLOCK: 如果这是一个阻塞socket, 操作将被block，perror输出: Resource temporarily unavailable

总结:
这个错误表示资源暂时不够，能read时，读缓冲区没有数据，或者write时，写缓冲区满了。遇到这种情况，如果是阻塞socket，
read/write就要阻塞掉。而如果是非阻塞socket，read/write立即返回-1， 同时errno设置为EAGAIN。
所以，对于阻塞socket，read/write返回-1代表网络出错了。但对于非阻塞socket，read/write返回-1不一定网络真的出错了。
可能是Resource temporarily unavailable。这时你应该再试，直到Resource available。

综上，对于non-blocking的socket，正确的读写操作为:
读：忽略掉errno = EAGAIN的错误，下次继续读
写：忽略掉errno = EAGAIN的错误，下次继续写

对于select和epoll的LT模式，这种读写方式是没有问题的。但对于epoll的ET模式，这种方式还有漏洞。


epoll的两种模式LT和ET
二者的差异在于level-trigger模式下只要某个socket处于readable/writable状态，无论什么时候进行epoll_wait都会返回该socket；
而edge-trigger模式下只有某个socket从unreadable变为readable或从unwritable变为writable时，epoll_wait才会返回该socket。

所以，在epoll的ET模式下，正确的读写方式为:
读：只要可读，就一直读，直到返回0，或者 errno = EAGAIN
写:只要可写，就一直写，直到数据发送完，或者 errno = EAGAIN

正确的读
n = 0;
while ((nread = read(fd, buf + n, BUFSIZ-1)) > 0) {
    n += nread;
}
if (nread == -1 && errno != EAGAIN) {
    perror("read error");
}
正确的写
int nwrite, data_size = strlen(buf);
n = data_size;
while (n > 0) {
    nwrite = write(fd, buf + data_size - n, n);
    if (nwrite < n) {
        if (nwrite == -1 && errno != EAGAIN) {
            perror("write error");
        }
        break;
    }
    n -= nwrite;
}
正确的accept，accept 要考虑 2 个问题
(1) 阻塞模式 accept 存在的问题
考虑这种情况：TCP连接被客户端夭折，即在服务器调用accept之前，客户端主动发送RST终止连接，导致刚刚建立的连接从就绪队列中移出，如果套接
口被设置成阻塞模式，服务器就会一直阻塞在accept调用上，直到其他某个客户建立一个新的连接为止。但是在此期间，服务器单纯地阻塞在accept调
用上，就绪队列中的其他描述符都得不到处理。

解决办法是把监听套接口设置为非阻塞，当客户在服务器调用accept之前中止某个连接时，accept调用可以立即返回-1，这时源自Berkeley的实现会在
内核中处理该事件，并不会将该事件通知给epool，而其他实现把errno设置为ECONNABORTED或者EPROTO错误，我们应该忽略这两个错误。

(2)ET模式下accept存在的问题
考虑这种情况：多个连接同时到达，服务器的TCP就绪队列瞬间积累多个就绪连接，由于是边缘触发模式，epoll只会通知一次，accept只处理一个连接，
导致TCP就绪队列中剩下的连接都得不到处理。

解决办法是用while循环抱住accept调用，处理完TCP就绪队列中的所有连接后再退出循环。如何知道是否处理完就绪队列中的所有连接呢？accept返回
-1并且errno设置为EAGAIN就表示所有连接都处理完。

综合以上两种情况，服务器应该使用非阻塞地accept，accept在ET模式下的正确使用方式为：
while ((conn_sock = accept(listenfd,(struct sockaddr *) &remote, (size_t *)&addrlen)) > 0) {
    handle_client(conn_sock);
}
if (conn_sock == -1) {
    if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
    perror("accept");
}
一道腾讯后台开发的面试题
使用Linuxepoll模型，水平触发模式；当socket可写时，会不停的触发socket可写的事件，如何处理？

第一种最普遍的方式：
需要向socket写数据的时候才把socket加入epoll，等待可写事件。
接受到可写事件后，调用write或者send发送数据。
当所有数据都写完后，把socket移出epoll。

这种方式的缺点是，即使发送很少的数据，也要把socket加入epoll，写完后在移出epoll，有一定操作代价。

一种改进的方式：
开始不把socket加入epoll，需要向socket写数据的时候，直接调用write或者send发送数据。如果返回EAGAIN，把socket加入epoll，在epoll的驱动下写数据，
全部数据发送完毕后，再移出epoll。

这种方式的优点是：数据不多的时候可以避免epoll的事件处理，提高效率。

最后贴一个使用epoll,ET模式的简单HTTP服务器代码:

*/