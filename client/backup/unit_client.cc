#include "unit_client.h"

#include "parser.h"

namespace proxy {
namespace unitclient {
	
	std::string NetClient::addr() const {
		return _addr;
	}

	std::string NetClient::id() const {
		return _id;
	}


	bool NetClient::init(boost::function<void(const request &)> fun) {
		if (fun) {
			_fun = fun;
		}else {
			_fun = boost::bind(&NetClient::handle_reply_detail, this, _1);
		}
		
	}

	bool NetClient::connect_server(const net::sock_addr &remoteaddr) {
		if ((_connfd = net::tcp_socket()) < 0){
			FLOG << "crate tcp socket error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
			return false;
		}
		//net::setnonblocking(_connfd);
		int32 optionVal = 0;
		::setsockopt(_connfd, SOL_SOCKET, SO_REUSEADDR, &optionVal, sizeof(optionVal));

		return net::connect(_connfd, remoteaddr);
	}

	void NetClient::handle_reply(int32 fd) {
		int32 n = ::read(_connfd, (void *)_tmpbuf.data(), MAXLINE);
		if (n == 0) {
			ELOG << "server terminated prematurely: [" << errno << ":" << ::strerror(errno) << "]";
		}

		for (;;) {//����һ���Խ����˶���������
			request req;
			req.fd = fd;
			bool flag = false;
			switch (PaserTool::paser(_tmpbuf, req)) {
			case PaserTool::kPaserError:
				::close(fd);//��epoll���Ƴ���������(���Ϊ�ȴ��رյ�����)
				flag = true;//�����������������forѭ��
				ELOG << "parser error";
				exit(1);
				break;
			case PaserTool::kPaserInprogress:
				flag = true;//�����������ݲ���һ����������forѭ��������������
				break;
			case PaserTool::kPaserHasdone:
				_fun(req);//ʹ��ע��Ļص���������
				flag = true;//�Ѿ�������������������forѭ��������������
				break;
			case PaserTool::kPaserHasleft:
				_fun(req);//ʹ��ע��Ļص���������
				break;
			default://unknow request,close connection
				::close(fd);
				exit(1);
				break;
			}

			if (flag) break;//����forѭ��
		}
	}

	void NetClient::handle_reply_detail(const proxy::request &reply) {
		//��ʱ��������
		//�����ܵ�������д�뵽��׼���������
		if (net::writen(fileno(stdout), reply.content.c_str(), reply.content.size()) < 0)
			ELOG << "write to stdout error: [" << errno << ":" << ::strerror(errno) << "]";
	}

	int32 NetClient::request_function(int32 fun_no, const std::string &data) {
		IPCPACK ptcol;
		::strncpy(ptcol.tag, "TKIPC", 5);
		::strncpy(ptcol.ueser_id, _id.c_str(), 16);
		ptcol.msg_type = 0;
		ptcol.flow_no = _flow_no++;
		ptcol.func_no = 10003;
		ptcol.err_no = 0;
		ptcol.body_len = data.size();
		int datalen = g_kIPCLen + ptcol.body_len;
		char *ptr_send_buf = new char[datalen + 1];
		memset(ptr_send_buf, '\0', datalen + 1);
		memcpy(ptr_send_buf, &ptcol, g_kIPCLen);
		memcpy(ptr_send_buf + g_kIPCLen, data.c_str(), ptcol.body_len);

		if (net::writen(_connfd, ptr_send_buf, datalen) < 0)
			ELOG << "write to server error: [" << errno << ":" << ::strerror(errno) << "]";

		return _flow_no - 1;
	}

	void NetClient::worker(){
		int maxfdp1;
		fd_set rset;
		char buf[MAXLINE];
		int n, flowno = 0;

		FD_ZERO(&rset);//������������0

		for (;;) {
			//�����׽���������
			FD_SET(_connfd, &rset);
			//�������������
			maxfdp1 = _connfd + 1;

			//����select�ȴ���������������Ӧ����������
			if (::select(maxfdp1, &rset, NULL, NULL, NULL) < 0) {
				if (errno == EINTR)//δ��ȡ���ݣ���������Ϊϵͳ�жϣ�
					continue;
				else
					ELOG << "select error: [" << errno << ":" << ::strerror(errno) << "]";
			}

			//�ж��Ƿ����׽��ֿɶ�
			if (FD_ISSET(_connfd, &rset)) {
				handle_reply(_connfd);
			}
		}
	}

	void NetClient::run() {
		_threadptr.reset(new safe::Thread(boost::bind(&NetClient::worker, this)));
		_threadptr->start();
	}
}
}//namespace epollthreadpool