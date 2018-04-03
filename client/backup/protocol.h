#pragma once

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

namespace proxy {
#pragma pack(push, 1)

typedef struct IPCPACK {
	char	tag[5];			//TKIPC
	char	ueser_id[16];	//�û�ID
	int32	msg_type;		//��Ϣ����(0:�����˳�,1:ҵ����Ϣ,2:�����̼�⵽�����ո���)
	int32	flow_no;		//��ˮ��
	uint32	func_no;		//���ܺ�
	int32	err_no;			//������(���������0)
	uint32	body_len;		//���ݳ���
}IPCPACK;

#pragma pack(pop)

typedef struct request {
	int32 fd;
	IPCPACK pack;
	std::string content;
}request;

typedef boost::shared_ptr<safe::Thread> ThreadPtr;
typedef boost::shared_ptr<request> RequestPtr;

static const std::string g_kIPCTag = "CCIPC";
static const int32 g_kIPCLen = sizeof(IPCPACK);

}//namespace epollthreadpool