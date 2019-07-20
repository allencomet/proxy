#ifndef __PROXY_CORE_PARSER_H
#define __PROXY_CORE_PARSER_H

#include "protocol.h"

namespace proxy {

    class PaserTool {
    public:
        typedef enum PASERSTATUS {
            kPaserError,
            kPaserInprogress,
            kPaserHasdone,
            kPaserHasleft
        } PASERSTATUS;

        static PASERSTATUS paser(StreamBuf &buf, request &req) {
            if (buf.size() < 5) return kPaserInprogress;

            if (0 == ::memcmp(buf.data(), g_kIPCTag.c_str(), 5)) {
                if (buf.size() < g_kIPCLen) {
                    return kPaserInprogress;
                } else {
                    IPCPACK *pack = (IPCPACK *) buf.data();
                    int32 expect_packlen = pack->body_len + g_kIPCLen;
                    if (expect_packlen < buf.size()) {
                        return kPaserInprogress;
                    } else {
                        req.type = kIpcPack;
                        ::memcpy(&req.ipc_pack, buf.data(), g_kIPCLen);
                        req.content.append(buf.data() + g_kIPCLen, pack->body_len);
                        buf.clear_head(expect_packlen);
                        if (0 == buf.size()) return kPaserHasdone;
                        else return kPaserHasleft;
                    }
                }
            } else if (0 == ::memcmp(buf.data(), g_kRPCTag.c_str(), 5)) {
                if (buf.size() < g_kRPCLen) {
                    return kPaserInprogress;
                } else {
                    RPCPACK *pack = (RPCPACK *) buf.data();
                    int32 expect_packlen = pack->body_len + g_kRPCLen;
                    if (expect_packlen < buf.size()) {
                        return kPaserInprogress;
                    } else {
                        req.type = kRpcPack;
                        ::memcpy(&req.rpc_pack, buf.data(), g_kRPCLen);
                        req.content.append(buf.data() + g_kRPCLen, pack->body_len);
                        buf.clear_head(expect_packlen);
                        if (0 == buf.size()) return kPaserHasdone;
                        else return kPaserHasleft;
                    }
                }
            } else {
                return kPaserError;
            }


        }

    private:
        PaserTool() {}

        ~PaserTool() {}

        DISALLOW_COPY_AND_ASSIGN(PaserTool);
    };

}//namespace epollthreadpool

#endif