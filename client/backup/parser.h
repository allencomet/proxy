#ifndef __EPOLL_THREADPOOL_PARSER_H
#define __EPOLL_THREADPOOL_PARSER_H

#include "protocol.h"

namespace proxy {

class PaserTool {
  public:
	typedef enum PASERSTATUS {
		kPaserError,
		kPaserInprogress,
		kPaserHasdone,
		kPaserHasleft
	}PASERSTATUS;

	static PASERSTATUS paser(StreamBuf &buf, request &req) {
		if (buf.size() < 5) {
			return kPaserInprogress;
		}

		//Э�鲻ƥ��
		if (0 != ::memcmp(buf.data(), g_kIPCTag.c_str(), 5)) {
			return kPaserError;
		}

		if (buf.size() < g_kIPCLen) {
			return kPaserInprogress;
		}else {
			IPCPACK *pack = (IPCPACK *)buf.data();
			int32 expect_packlen = pack->body_len + g_kIPCLen;
			if (expect_packlen < buf.size()) {//��δ������
				return kPaserInprogress;
			}else {
				::memcpy(&req.pack, buf.data(), g_kIPCLen);
				req.content.assign(buf.data() + g_kIPCLen, pack->body_len);
				buf.clear_head(expect_packlen);
				if (0 == buf.size()) {
					return kPaserHasdone;
				}else {
					return kPaserHasleft;
				}
			}
		}
	}

  private:
	PaserTool() {}
	~PaserTool() {}

	DISALLOW_COPY_AND_ASSIGN(PaserTool);
};

}//namespace epollthreadpool

#endif