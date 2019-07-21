#include "rpc.h"

#include "dispatcher_rpc.h"

namespace {
    int32 init_tcp_socket(const std::string &ip, const int16 &port) {
        int32 listenfd = net::tcp_socket();
        net::setnonblocking(listenfd);
        int32 optionVal = 0;
        ::setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optionVal, sizeof(optionVal));

        FLOG_IF(listenfd < 0) << "crate tcp socket error has been occurred: [" << errno << ":" << ::strerror(errno)
                              << "]";

        net::ipv4_addr localaddr(ip, port);
        if (!net::bind(listenfd, localaddr)) {
            return -1;
        }

        if (!net::listen(listenfd, 20)) {
            return -1;
        }

        return listenfd;
    }
}

namespace proxy {
    namespace rpc {
        NetServer::NetServer(const std::string &ip, const int16 &port)
                : _ip(ip),
                  _port(port) {

            // create socket
            _listenfd = init_tcp_socket(ip, port);

            // initialize the thread pool by listening on the binding port
            for (int16 i = 0; i < ::FLG_thcount; ++i) {
                ThreadPtr ptr(new safe::Thread(std::bind(&NetServer::worker, this, _listenfd)));
                _threads.push_back(ptr);
            }
        }

        void NetServer::run() {

            // start the thread pool to work
            for (auto it = _threads.begin();
                 it != _threads.end(); ++it) {
                (*it)->start();
            }

            // the main thread waits for the child to exit
            for (;;) {
                if (!::FLG_stop_server) sys::sleep(3);
                else break;
            }

            // alert the user that the program is about to exit
            std::string msg("stop running server\n");
            ::fwrite(msg.data(), 1, msg.size(), stderr);

            // recycle the thread pool
            for (auto it = _threads.begin();
                 it != _threads.end(); ++it) {
                (*it)->cancel();
                (*it)->join();
            }

            msg = "server has been stopped\n";
            ::fwrite(msg.data(), 1, msg.size(), stderr);
        }

        // listens on the specified port and accepts client connections
        void NetServer::worker(int32 listenfd) {
            LOG << "NetServer listen on port: " << _port;
            Dispatcher dispatcher(_ip, _port);
            dispatcher.init(listenfd);
            dispatcher.run();
        }

    }//namespace rpc
}//namespace proxy