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
	char	ueser_id[16];	//用户ID
	int32	msg_type;		//消息类型(0:进程退出,1:业务消息,2:主进程检测到交易日更新)
	int32	flow_no;		//流水号
	uint32	func_no;		//功能号
	int32	err_no;			//错误码(请求包设置0)
	uint32	body_len;		//数据长度
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
static const int32 g_kPackLen = sizeof(IPCPACK);

}//namespace epollthreadpool