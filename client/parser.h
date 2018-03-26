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

		//协议不匹配
		if (0 != ::memcmp(buf.data(), g_kIPCTag.c_str(), 5)) {
			return kPaserError;
		}

		if (buf.size() < g_kPackLen) {
			return kPaserInprogress;
		}else {
			IPCPACK *pack = (IPCPACK *)buf.data();
			int32 expect_packlen = pack->body_len + g_kPackLen;
			if (expect_packlen < buf.size()) {//包未收完整
				return kPaserInprogress;
			}else {
				::memcpy(&req.pack, buf.data(), g_kPackLen);
				req.content.assign(buf.data() + g_kPackLen, pack->body_len);
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