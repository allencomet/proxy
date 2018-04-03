#ifndef __EPOLL_THREADPOOL_CLIENT_H
#define __EPOLL_THREADPOOL_CLIENT_H

#include "../util/util.h"

#include "protocol.h"

namespace proxy {
namespace unitclient {

	class UnitClient {
	public:
		UnitClient() {}
		virtual ~UnitClient() {}

		virtual std::string addr() const = 0;
		virtual std::string id() const = 0;
		virtual bool init(boost::function<void(const request &)> fun) = 0;//注册接收到响应包的回调
		virtual bool connect_server(const net::sock_addr&) = 0;
		virtual void run() = 0;
		
	private:

		DISALLOW_COPY_AND_ASSIGN(UnitClient);
	};

	class NetClient : public UnitClient {
	public:
		NetClient(const std::string id) :_id(id), _connfd(0), _flow_no(0){}

		~NetClient() {
			if (_threadptr){
				_threadptr->cancel();
				_threadptr->join();
			}
		}

		virtual std::string addr() const;
		virtual std::string id() const;
		virtual bool init(boost::function<void(const request &)> fun);
		virtual bool connect_server(const net::sock_addr &);
		virtual void run();


		int32 request_function(int32 fun_no, const std::string &data);
	private:
		void worker();
		void handle_reply(int32 fd);
		void handle_reply_detail(const request &reply);


		StreamBuf _tmpbuf;
		int32 _flow_no;
		int32 _connfd;
		std::string _addr;
		std::string _id;
		boost::function<void(const request &)>	_fun;
		ThreadPtr _threadptr;

		DISALLOW_COPY_AND_ASSIGN(NetClient);
	};


}//namespace unitclient
}//namespace epollthreadpool

#endif