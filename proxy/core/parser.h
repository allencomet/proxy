#ifndef __PROXY_CORE_PARSER_H
#define __PROXY_CORE_PARSER_H

#include "protocol.h"

namespace proxy {

    class ParseTool {
    public:
        typedef enum PASERSTATUS {
            kParseError,
            kParseInProgress,
            kParseHasDone,
            kParseHasLeft
        } PASERSTATUS;

        static PASERSTATUS parse(StreamBuf &buf, request &req) {
            if (buf.size() < 5) return kParseInProgress;

            if (0 == ::memcmp(buf.data(), g_kIPCTag.c_str(), 5)) {
                if (buf.size() < g_kIPCLen) {
                    return kParseInProgress;
                } else {
                    IPCPACK *pack = (IPCPACK *) buf.data();
                    int32 expect_packlen = pack->body_len + g_kIPCLen;
                    if (expect_packlen < buf.size()) {
                        return kParseInProgress;
                    } else {
                        req.type = kIpcPack;
                        ::memcpy(&req.ipc_pack, buf.data(), g_kIPCLen);
                        req.content.append(buf.data() + g_kIPCLen, pack->body_len);
                        buf.clear_head(expect_packlen);
                        if (0 == buf.size()) return kParseHasDone;
                        else return kParseHasLeft;
                    }
                }
            } else if (0 == ::memcmp(buf.data(), g_kRPCTag.c_str(), 5)) {
                if (buf.size() < g_kRPCLen) {
                    return kParseInProgress;
                } else {
                    RPCPACK *pack = (RPCPACK *) buf.data();
                    int32 expect_packlen = pack->body_len + g_kRPCLen;
                    if (expect_packlen < buf.size()) {
                        return kParseInProgress;
                    } else {
                        req.type = kRpcPack;
                        ::memcpy(&req.rpc_pack, buf.data(), g_kRPCLen);
                        req.content.append(buf.data() + g_kRPCLen, pack->body_len);
                        buf.clear_head(expect_packlen);
                        if (0 == buf.size()) return kParseHasDone;
                        else return kParseHasLeft;
                    }
                }
            } else {
                return kParseError;
            }


        }

    private:
        ParseTool() {}

        ~ParseTool() {}

        DISALLOW_COPY_AND_ASSIGN(ParseTool);
    };

}//namespace epollthreadpool

#endif