#include "server.h"

#include "client.h"
#include "reply.h"

namespace selectsrv{

	//进程间通信消息头
	typedef struct{
		char	m_szUserID[16];	//用户ID
		int32	m_nMsgType;		//消息类型(0:进程退出,1:业务消息,2:主进程检测到交易日更新)
		int32	m_nFlowNo;	//流水号
		uint32	m_nFuncNo;	//功能号
		int32	m_nErrNo;	//错误码(请求包设置0)
		uint32	m_nDataLen;	//数据长度
	}EMsgInfo;

	server::~server(){
		for (boost::unordered_map<int32,client *>::iterator it = _clients.begin();
			it != _clients.end(); ++it){
				if (NULL != it->second){
					delete it->second;
					it->second = NULL;
				}
		}
		_clients.clear();
	}

	void server::init_server(){
		unlink(_path.c_str());
		net::unix_addr addr(_path);

		if ((_listenfd = net::unix_socket()) < 0){
			errorlog::err_sys("[init_server]: unix_socket error");
		}
		
		if (!net::bind(_listenfd,addr)){
			errorlog::err_sys("[init_server]: bind error");
		}

		if (!net::listen(_listenfd)){
			errorlog::err_sys("[init_server]: listen error");
		}

		errorlog::err_msg("bind success,address: %s",addr.to_string().c_str());
	}


	void server::worker(){
		init_server();
		ssize_t read_count = 0, write_count = 0;
		int32 i,maxi,maxfd,connfd,sockfd;
		int32 nready,client_arr[FD_SETSIZE];
		fd_set rset, wset;
		socklen_t clilen;
		struct sockaddr_un cliaddr;
		

		//初始状态下，监听套接字描述符值就是最大的描述符值
		//（因为只有fd0\fd1\fd2代表着标准输入、标准输出、标准错误，那么其他的描述符只能从fd3开始）
		maxfd = _listenfd;//代表着描述符集中开启的最大描述符值
		maxi = -1;//最多能容纳的套接字

		//初始化client数组
		for(i=0;i<FD_SETSIZE;i++)
			client_arr[i]=-1;

		//初始化描述符集
		FD_ZERO(&_rset);
		FD_ZERO(&_wset);
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		//开启描述符集中监听套接字对应的位
		FD_SET(_listenfd,&_rset);

		for (;;){
			rset = _rset;
			wset = _wset;
			//等待rset描述符集中的描述符可读
			if((nready = select(maxfd+1, &rset, &wset, NULL, NULL)) < 0){
				if(errno == EINTR)	
					continue;       //系统中断，重试
				else
					errorlog::err_sys("[worker]: select error");
			}

			//判断是否是监听套接字可读
			if(FD_ISSET(_listenfd, &rset)){
				if((connfd = accept(_listenfd,(struct sockaddr *)&cliaddr,&clilen)) < 0)
					errorlog::err_sys("[worker]: accept error");

				//保存已连接的描述符
				for(i = 0; i < FD_SETSIZE; i++){
					if(client_arr[i] < 0){
						client_arr[i] = connfd;
						break;
					}
				}

				//新建一个用户
				selectsrv::client *pcli = new selectsrv::client(connfd,_request_handler,_request_parser);
				_clients.insert(std::make_pair(connfd,pcli));
				
				//客户进程描述符超出最大的描述符值
				if(i == FD_SETSIZE)
					errorlog::err_quit("[worker]: too many clients");

				//在所有描述符集中开启已连接套接字对应的位
				FD_SET(connfd, &_rset);
				//如果已连接套接字描述符值大于maxfd，那么将connfd值设位最大描述符值（从0开始，比如fd0）
				maxfd = MAX(connfd, maxfd);

				//如果下标i超出了最大索引，那么将i设置为最大索引
				maxi = MAX(i, maxi);

				//如果只有一个监听套接字准备好，那么等待其他描述符变为可读
				if(--nready <= 0)
					continue;
			}

			//检查来自客户进程数据
			for(i = 0; i <= maxi; ++i){
				//遍历直到遇到第一个已连接的套接字(>=0)
				if((sockfd = client_arr[i]) < 0)
					continue;

				//判断是否是套接字描述符可读
				if(FD_ISSET(sockfd, &rset)){
					selectsrv::client *pcli = get_client(sockfd);
					if (NULL != pcli){
						char recv_buf[MAXLINE] = { '\0' };
						if((read_count = net::read(sockfd, (void *)recv_buf, MAXLINE)) <= 0){
							close_client(sockfd);//如果读取到文件尾，那么关闭这个已连接的套接字
							client_arr[i] = -1;//并且在保存已连接套接字描述符的数组中初始化对应的索引
							errorlog::err_msg("read error has occured");
						}else{
							pcli->rdata(recv_buf, read_count);
							if (!pcli->handle_request()){//asynchronous handle
								close_client(sockfd);//如果读取到文件尾，那么关闭这个已连接的套接字
								client_arr[i] = -1;//并且在保存已连接套接字描述符的数组中初始化对应的索引
								errorlog::err_msg("handle_request error has occured");
							}
						}

						//如果没有可读的套接字描述符，那么跳出循环
						if(--nready<=0) break;
					}
				}

				//if (FD_ISSET(sockfd, &wset)){
				//	selectsrv::client *pcli = get_client(sockfd);
				//	if (NULL != pcli) {
				//		StreamBuf &reply_msg = pcli->wbuf();
				//		if (0 < reply_msg.size()){
				//			write_count = net::write(pcli->fd(), (const void *)reply_msg.data(), reply_msg.size());
				//			if (write_count < 0) {
				//				if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
				//					FD_SET(sockfd, &_wset);//continue to writing
				//				}else {
				//					close_client(sockfd);//如果读取到文件尾，那么关闭这个已连接的套接字
				//					client_arr[i] = -1;//并且在保存已连接套接字描述符的数组中初始化对应的索引
				//					errorlog::err_msg("write error has occured");
				//				}
				//			}else if (write_count == 0) {
				//				close_client(sockfd);//如果读取到文件尾，那么关闭这个已连接的套接字
				//				client_arr[i] = -1;//并且在保存已连接套接字描述符的数组中初始化对应的索引
				//				errorlog::err_msg("closed by peer");
				//			}else {
				//				reply_msg.clear_head(write_count);
				//			}
				//		}else {
				//			errorlog::err_msg("all data has been send");
				//			FD_CLR(sockfd, &_wset);//在所有描述符集中清除这个描述符对应的位
				//		}
				//	}
				//}

			}//for
		}//for(;;)
	}

	//bool server::handle_request(client *pcli){
	//	safebuf &cli_inbuf = pcli->inbuf();
	//	errorlog::err_msg("handle request %d bytes data: %s",cli_inbuf.size(),cli_inbuf.to_string().c_str());

	//	safebuf &reply_msg = pcli->outbuf();
	//	reply_msg << "handle request " << cli_inbuf.size() << " bytes data,I'm tied up at work,talk to you then.";
	//	cli_inbuf.clear_head(cli_inbuf.size());

	//	int32 write_count = net::write(pcli->fd(), (const void *)reply_msg.data(), reply_msg.size());
	//	if (write_count > 0){
	//		reply_msg.clear_head(write_count);
	//		if (!reply_msg.empty()){
	//			FD_SET(pcli->fd(), &_wset);//continue to writing
	//		}
	//	}

	//	errorlog::err_msg("has been send [%d] bytes data,out buffer left [%d] bytes data.", write_count, reply_msg.size());

	//	for (;;){
	//		request req;
	//		bool bre = false;
	//		switch (_request_parser.parser(cli_inbuf, req)) {
	//		case ERROR:
	//			bre = true;
	//			break;
	//		case INPROGRESS:
	//			bre = true;
	//			break;
	//		case HASDONE: 
	//			{
	//				reply rep;
	//				_request_handler.handle_request(req, rep);

	//			}
	//			bre = true;
	//			break;
	//		case HASLEFT:
	//			break;
	//		default:
	//			break;
	//		}

	//		if (bre) break;
	//	}


	//	return true;
	//}


}