#pragma once

#include <cstdio>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>

#include "../../util/util.h"

namespace proxy {
#pragma pack(push, 1)

    typedef enum {
        BACK_EXIT,
        BACK_MSG
    } IPCMSGTYPE;

    typedef struct {
        char tag[5];            //CCIPC
        char ueser_id[16];    //用户ID
        int32 msg_type;        //消息类型(0:进程退出,1:业务消息)
        int32 flow_no;        //流水号
        uint32 func_no;        //功能号
        int32 err_no;            //错误码(请求包设置0)
        uint32 body_len;        //数据长度
    } IPCPACK;

    typedef struct {
        char tag[5];            //CCRPC
        uchar body_type;        //包体数据类型（'0':json格式,'1':二进制格式）
        uchar msg_type;        //消息安全类型(0:正常,1:加密,2:压缩,3:压缩且加密)
        int32 flow_no;        //流水号
        uint32 func_no;        //功能号
        int32 err_no;            //错误码(请求包设置0)
        uint32 body_len;        //数据长度
        uint32 origin_len;        //原数据长度
        char reserved[20];    //保留字段
    } RPCPACK;

    typedef struct {
        char tag[5];            //SPRPC
        char topic[255];        //订阅主题名
    } SUBPACK;

#pragma pack(pop)

//const对象默认为文件的局部变量，所以const变量可以在头文件定义
    const std::string g_kIPCTag = "CCIPC";
    const std::string g_kRPCTag = "CCRPC";
    const int32 g_kIPCLen = sizeof(IPCPACK);
    const int32 g_kRPCLen = sizeof(RPCPACK);

    typedef enum PACKTYPE {
        kUnKnow,
        kIpcPack,                //IPC通信协议类型
        kRpcPack                //RPC通信协议类型
    } PACKTYPE;

    typedef struct request {
        request() : fd(0), type(kUnKnow) {
        }

        request(const request &r) {
            fd = r.fd;
            type = r.type;
            switch (r.type) {
                case kIpcPack:
                    ::memcpy(&ipc_pack, &r.ipc_pack, g_kIPCLen);
                    break;
                case kRpcPack:
                    ::memcpy(&rpc_pack, &r.rpc_pack, g_kRPCLen);
                    break;
                default:
                    ::memcpy(&rpc_pack, &r.rpc_pack, g_kRPCLen);
                    break;
            }
            content.clear();
            content.append(r.content.data(), r.content.size());
        }

        request &operator=(const request &r) {
            fd = r.fd;
            type = r.type;
            switch (r.type) {
                case kIpcPack:
                    ::memcpy(&ipc_pack, &r.ipc_pack, g_kIPCLen);
                    break;
                case kRpcPack:
                    ::memcpy(&rpc_pack, &r.rpc_pack, g_kRPCLen);
                    break;
                default:
                    ::memcpy(&rpc_pack, &r.rpc_pack, g_kRPCLen);
                    break;
            }
            content.clear();
            content.append(r.content.data(), r.content.size());
            return *this;
        }

        int32 fd;
        PACKTYPE type;
        union {
            IPCPACK ipc_pack;
            RPCPACK rpc_pack;
        };
        StreamBuf content;
    } request;

    typedef struct ErrMsg {
        ErrMsg(int32 err, const std::string msg) : err_no(err), err_msg(msg) {}

        int32 err_no;
        std::string err_msg;
    } ErrMsg;

    typedef std::shared_ptr<safe::Thread> ThreadPtr;
    typedef std::shared_ptr<request> RequestPtr;


}//namespace proxy