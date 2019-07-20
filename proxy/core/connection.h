#ifndef __PROXY_CORE_CONNECTION_H
#define __PROXY_CORE_CONNECTION_H

#include "../../util/util.h"


namespace proxy {

    typedef std::shared_ptr<StreamBuf> StreamBufPtr;

    class Connection {
    public:
        Connection(int32 fd) : _fd(fd) {}

        Connection(int32 fd, const std::string &addr) : _fd(fd), _complete_addr(addr) {}

        ~Connection() {}

        inline int32 fd() const {
            return _fd;
        }

        std::string addr() const {
            return _complete_addr;
        }

        void set_addr(const std::string &addr) {
            _complete_addr = addr;
        }

        StreamBuf &rbuf() {
            return _rbuf;
        }

        /*StreamBuf &wbuf() {
            return _wbuf;
        }*/

        /*int32 wbuf_size() {
            safe::MutexGuard g(_wmutex);
            return _wbuf.size();
        }*/

        inline void append_rbuf_s(const char *data, int32 n) {
            safe::MutexGuard g(_rmutex);
            _rbuf.append(data, n);
        }

        inline void append_write_pack_s(StreamBufPtr ptr) {
            safe::MutexGuard g(_wmutex);
            _wqueue.push_back(ptr);
        }

        inline void get_write_pack_s(std::deque<StreamBufPtr> &que) {
            safe::MutexGuard g(_wmutex);
            std::swap(que, _wqueue);
        }

        /*void safe_append_wbuf(const char *data, int32 n) {
            safe::MutexGuard g(_wmutex);
            _wbuf.append(data, n);
        }*/

        /*void safe_clear_head_wbuf(int32 n) {
            safe::MutexGuard g(_wmutex);
            _wbuf.clear_head(n);
        }*/

        void append_rbuf_ns(const char *data, int32 n) {
            _rbuf.append(data, n);
        }

        /*void unsafe_append_wbuf(const char *data, int32 n) {
            _wbuf.append(data, n);
        }*/

        inline void set_msg_time(int64 t) {
            _time = t;
        }

        inline int64 msg_time() const {
            return _time;
        }

        inline safe::Mutex &wmutex() {
            return _wmutex;
        }

    private:
        int32 _fd;
        int64 _time;//last msg time
        std::string _complete_addr;//保存完整地址，可能是网络地址，也可能是路径名


        safe::Mutex _rmutex;
        StreamBuf _rbuf;

        safe::Mutex _wmutex;
        std::deque<StreamBufPtr> _wqueue;//响应消息队列
        //StreamBuf _wbuf;

        DISALLOW_COPY_AND_ASSIGN(Connection);
    };

    typedef std::shared_ptr<Connection> ConnectionPtr;

}
#endif