#include "request_parser.h"

namespace selectsrv {
	request_parser::request_parser() {}
	request_parser::~request_parser() {}

	parser_status request_parser::parser(StreamBuf &buf, cli_req &req) {
		int32 buf_len = buf.size();
		if (buf_len < PROTOCOL_LEN){
			return INPROGRESS;
		}else {
			protocol *ptr = (protocol *)buf.data();
			if (buf_len == (ptr->_bodylen + PROTOCOL_LEN)){
				::memcpy(&req._protocol, buf.data(), PROTOCOL_LEN);
				req._body.assign(buf.data(PROTOCOL_LEN), ptr->_bodylen);
				buf.clear();
				return HASDONE;
			}else if (buf_len > (ptr->_bodylen + PROTOCOL_LEN)) {
				::memcpy(&req._protocol, buf.data(), PROTOCOL_LEN);
				req._body.assign(buf.data(PROTOCOL_LEN), ptr->_bodylen);
				buf.clear_head(ptr->_bodylen + PROTOCOL_LEN);
				return HASLEFT;
			}else {
				return INPROGRESS;
			}
		}
	}
}