#ifndef __SERVER_H
#define __SERVER_H

#include "../util/util.h"

#include "request_handler.h"
#include "request_parser.h"

namespace selectsrv{
	class client;
	class server{
	public:
		server(const std::string &path)
			:_path(path),_listenfd(0){
		}
		~server();


		void run(){
			safe::Thread thread(boost::bind(&server::worker,this));
			thread.start();
			thread.join();
		}

	private:
		void init_server();
		void worker();

		client *get_client(int32 fd){
			boost::unordered_map<int32,client *>::iterator it = _clients.find(fd);
			if (_clients.end() == it) return NULL;
			else return it->second;
		}

		void close_client(int32 fd){
			close(fd);
			FD_CLR(fd, &_rset);//在所有描述符集中清除这个描述符对应的位
			FD_CLR(fd, &_wset);//在所有描述符集中清除这个描述符对应的位
			boost::unordered_map<int32,client *>::iterator it = _clients.find(fd);
			if (_clients.end() == it) return;
			else _clients.erase(it);
		}

		//bool handle_request(client *pcli);
	private:
		std::string _path;
		int32 _listenfd;
		fd_set _rset,_wset;

		boost::unordered_map<int32,client *> _clients;

		request_handler _request_handler;
		request_parser _request_parser;

		DISALLOW_COPY_AND_ASSIGN(server);
	};
}

#endif