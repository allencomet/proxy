#include "../util/util.h"
#include "../proxy/core/protocol.h"


#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

namespace client {

	int ipc_client(const std::string &path) {
		//创建UNIX域套接字
		int sockfd = net::udp_socket();
		CHECK_GE(sockfd, 0) << "create unix socket failed..";

		net::unix_addr serv_addr(path);
		CHECK(net::connect(sockfd, serv_addr));
			
		return sockfd;
	}

	int rpc_client(const std::string &addr,int port) {
		//创建TCP套接字
		int sockfd = net::tcp_socket();
		CHECK_GE(sockfd, 0) << "create tcp socket failed..";

		//设置服务器地址
		net::ipv4_addr serv_addr(addr, port);

		CHECK(net::connect(sockfd, serv_addr));

		net::ipv4_addr remote, local;
		net::getpeername(sockfd, &remote);
		net::getsockname(sockfd, &local);

		CERR << "client, sock: " << local.to_string()
			<< ", peer: " << remote.to_string();

		return sockfd;
	}

	void start_ipc_client(int sockfd) {
		int maxfdp1, stdineof;
		fd_set rset;
		char buf[MAXLINE];
		int n, flowno = 0;

		stdineof = 0;//判断是否文件是否可读
		FD_ZERO(&rset);//将描述符集置0

		char send_msg[] = "hello everyone,I'm allen, I'm gratuated to give my showcase here!";

		proxy::IPCPACK pack;
		::memset(&pack, '\0', proxy::g_kIPCLen);
		::memcpy(pack.tag, proxy::g_kIPCTag.c_str(), proxy::g_kIPCTag.size());
		::memcpy(pack.ueser_id, "allen", 5);
		pack.msg_type = 0;
		pack.flow_no = flowno;
		pack.func_no = 3000;
		pack.err_no = 0;
		pack.body_len = ::strlen(send_msg);
		int datalen = proxy::g_kIPCLen + pack.body_len;
		StreamBuf sendbuf(datalen);
		sendbuf.append(&pack, proxy::g_kIPCLen);
		sendbuf.append(send_msg, pack.body_len);

		if (net::writen(sockfd, sendbuf.data(), datalen) < 0) {
			CERR << "write error";
			return;
		}

		for (;;) {
			//开启套接字描述符
			FD_SET(sockfd, &rset);
			//计算最大描述符
			maxfdp1 = sockfd + 1;

			//利用select等待描述符集中有相应描述符可用
			if (::select(maxfdp1, &rset, NULL, NULL, NULL)<0) {
				if (errno == EINTR)//未读取数据，可能是因为系统中断，
					continue;
				else
					CERR << "select error";
			}

			//判断是否是套接字可读
			if (FD_ISSET(sockfd, &rset)) {
				if ((n = net::read(sockfd, buf, MAXLINE)) == 0) {
					if (stdineof == 1) {//读取到文件尾
						return;
					} else {
						CERR << "str_cli: server terminated prematurely";
						break;
					}
				}

				//将接受到的数据写入到标准输出描述符
				if (net::writen(fileno(stdout), buf, n) < 0) {
					CERR << "writen error";
					break;
				}

				proxy::IPCPACK *packptr = (proxy::IPCPACK *)sendbuf.data();
				packptr->flow_no++;

				if (net::writen(sockfd, sendbuf.data(), datalen) < 0) {
					CERR << "writen error";
					break;
				}
			}

			//break;
		}
	}

	void start_rpc_client(int sockfd) {
		int maxfdp1, stdineof;
		fd_set rset;
		char buf[MAXLINE];
		int n, flowno = 0;

		stdineof = 0;//判断是否文件是否可读
		FD_ZERO(&rset);//将描述符集置0

		std::string send_msg("{\"user_id\":\"allen\"}");

		proxy::RPCPACK pack;
		::memset(&pack, '\0', proxy::g_kRPCLen);
		::memcpy(pack.tag, proxy::g_kRPCTag.c_str(), proxy::g_kRPCTag.size());
		pack.msg_type = 0;
		pack.flow_no = flowno;
		pack.func_no = 1000;
		pack.err_no = 0;
		pack.body_len = send_msg.size();

		int datalen = proxy::g_kRPCLen + pack.body_len;
		StreamBuf sendbuf(datalen);
		sendbuf.append(&pack, proxy::g_kRPCLen);
		sendbuf.append(send_msg.c_str(), send_msg.size());

		CERR << "send " << datalen << " msg to server";

		if (net::writen(sockfd, sendbuf.data(), datalen) < 0) {
			CERR << "write error";
			return;
		}
			

		for (;;) {
			//开启套接字描述符
			FD_SET(sockfd, &rset);
			//计算最大描述符
			maxfdp1 = sockfd + 1;

			//利用select等待描述符集中有相应描述符可用
			if (::select(maxfdp1, &rset, NULL, NULL, NULL) < 0) {
				if (errno == EINTR) {
					continue;
				} else {
					CERR << "select error";
					break;
				}
					
			}

			//判断是否是套接字可读
			if (FD_ISSET(sockfd, &rset)) {
				if ((n = net::read(sockfd, buf, MAXLINE)) == 0) {
					if (stdineof == 1) {//读取到文件尾
						return;
					} else {
						CERR << "str_cli: server terminated prematurely";
						break;
					}
				}

				proxy::RPCPACK *pack = (proxy::RPCPACK *)sendbuf.data();

				std::string content(buf + proxy::g_kRPCLen, n - proxy::g_kRPCLen);
				CERR << "func_no: " << pack->func_no << ",flow_no: " << pack->flow_no << ",content: " << content;

				pack->flow_no++;

				if (net::writen(sockfd, sendbuf.data(), datalen) < 0) {
					CERR << "writen error";
					break;
				}
					
				::memset(buf, 0, MAXLINE);
			}

			//break;
		}
	}
}



//int main(int argc,char **argv){
//	int sockfd = net_client(argc, argv);
//	//int sockfd = ipc_client(argc, argv);
//	str_cli_rpc(sockfd);
//	exit(0);
//}


