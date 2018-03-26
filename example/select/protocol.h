#ifndef __PROTOCOL_H
#define __PROTOCOL_H
#include "../util/util.h"

namespace selectsrv {
#pragma pack(push, 1)
	typedef struct {
		char	_ueser_id[16];	//用户ID
		int32	_msg_type;		//消息类型(0:进程退出,1:业务消息,2:主进程检测到交易日更新)
		int32	_flow_no;		//流水号
		uint32	_func_no;		//功能号
		int32	_errorno;		//错误码(请求包设置0)
		uint32	_bodylen;		//数据长度
	}protocol;
#pragma pack(pop)

	static const int32 PROTOCOL_LEN = sizeof(protocol);
}

#endif