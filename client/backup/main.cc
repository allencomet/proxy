#include "../util/util.h"

#include "unit_client.h"


#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

#define UNIXSTR_PATH "/root/epoll_wrap/debug/bin/mastersrv"

void handle_reply(const proxy::request &reply) {
	//暂时不做处理
	//将接受到的数据写入到标准输出描述符
	if (net::writen(fileno(stdout), reply.content.c_str(), reply.content.size()) < 0)
		ELOG << "write to stdout error: [" << errno << ":" << ::strerror(errno) << "]";
}

int main(int argc,char **argv){
	net::ipv4_addr remoteaddr("127.0.0.1", 8080);
	boost::function<void(const proxy::request &)> fun;
	proxy::unitclient::NetClient cli("allen");
	cli.init(fun);
	cli.connect_server(remoteaddr);
	cli.run();

	for (;;){
		cli.request_function(10003, "hello");
	}

	return 0;
}

//int ipc_client(int argc, char **argv) {
//	int sockfd;
//	struct sockaddr_un servaddr;
//
//	//创建UNIX域套接字
//	if ((sockfd = ::socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
//		err_sys("socket error ");
//
//	//初始化并且填充UNIX域套接字
//	bzero(&servaddr, sizeof(servaddr));
//	servaddr.sun_family = AF_UNIX;
//	::strcpy(servaddr.sun_path, UNIXSTR_PATH);
//
//	//连接对端套接字
//	if (::connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
//		err_sys("connect error");
//
//	return sockfd;
//}
//
//int net_client(int argc, char **argv) {
//	int sockfd;
//	struct sockaddr_in servaddr;        //套接字地址结构
//
//	if (argc != 3)
//		err_quit("usuage: %s <IPaddress> <Port>",argv[0]);
//
//	if ((sockfd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)    //创建套接字
//		err_quit("socket error");
//
//	bzero(&servaddr, sizeof(servaddr));  //初始化套接字
//	servaddr.sin_family = AF_INET;
//	servaddr.sin_port = ::htons(::atoi(argv[2]));     //设置端口号
//
//	if (::inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0)     //将字符串格式转换成网络字节序格式
//		err_quit("inet_pton error");
//
//	if (::connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)     //连接到服务端
//		err_quit("connect error");
//
//	return sockfd;
//}
//
//ssize_t writen(int fd,const void *vptr,size_t n){
//	size_t nleft;	//剩余量
//	ssize_t nwritten;	//已写入量
//	const char *ptr;
//
//	ptr=(const char *)vptr;
//	nleft=n;
//	while(nleft>0){
//		if( (nwritten=write(fd,ptr,nleft)) <=0 ){
//			if( nwritten<0 && errno==EINTR )		//如果是因为系统捕捉信号和中断，那么write重试
//				nwritten=0;
//			else
//				return -1;
//		}
//
//		nleft-=nwritten;
//		ptr+=nwritten;
//			
//	}
//
//	return n;
//	
//}
//
//void str_cli(int sockfd){
//	int maxfdp1,stdineof;
//	fd_set rset;
//	char buf[MAXLINE];
//	int n, flowno = 0;
//
//	stdineof=0;//判断是否文件是否可读
//	FD_ZERO(&rset);//将描述符集置0
//
//	char send_msg[] = "hello everyone,I'm allen, I'm gratuated to give my showcase here!";
//
//	IPCPACK ptcol;
//	::strncpy(ptcol._tag,"TKIPC",5);
//	::strncpy(ptcol._ueser_id,"allen",5);
//	ptcol._msg_type = 0;
//	ptcol._flow_no = flowno++;
//	ptcol._func_no = 10003;
//	ptcol._errorno = 0;
//	ptcol._bodylen = ::strlen(send_msg);
//	int datalen = PROTOCOL_LEN + ptcol._bodylen;
//	char *ptr_send_buf = new char[datalen + 1];
//	memset(ptr_send_buf,'\0',datalen + 1);
//	memcpy(ptr_send_buf,&ptcol,PROTOCOL_LEN);
//	memcpy(ptr_send_buf+PROTOCOL_LEN,send_msg,ptcol._bodylen);
//
//	if(writen(sockfd,ptr_send_buf,datalen)<0)
//		err_sys("write error");
//
//	for(;;){
//		//开启套接字描述符
//		FD_SET(sockfd,&rset);
//		//计算最大描述符
//		maxfdp1= sockfd +1;
//
//		//利用select等待描述符集中有相应描述符可用
//		if(::select(maxfdp1,&rset,NULL,NULL,NULL)<0){
//			if(errno==EINTR)//未读取数据，可能是因为系统中断，
//				continue;
//			else
//				err_sys("select error");
//		}
//
//		//判断是否是套接字可读
//		if(FD_ISSET(sockfd,&rset)){
//			if((n=read(sockfd,buf,MAXLINE))==0){
//				if(stdineof==1)//读取到文件尾
//					return ;
//				else
//					err_quit("str_cli: server terminated prematurely");
//			}
//
//			//将接受到的数据写入到标准输出描述符
//			if(writen(fileno(stdout),buf,n)<0)
//				err_sys("writen error");
//
//			
//			IPCPACK *pack = (IPCPACK *)ptr_send_buf;
//			pack->_flow_no++;
//			if(writen(sockfd,ptr_send_buf,datalen)<0)
//				err_sys("writen error");
//		}
//	}
//}
