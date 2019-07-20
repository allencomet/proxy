#ifndef __PROXY_IPC_REQUEST_HANDLER_H
#define __PROXY_IPC_REQUEST_HANDLER_H

#include "../../util/util.h"
#include "../core/protocol.h"

namespace proxy {

    class ConnManager;

    namespace ipc {

        class Dispatcher;

        class RequestHandler {
        public:
            RequestHandler(Dispatcher &epollcore, ConnManager &manager, int32 n = 4)
                    : _epollcore(epollcore), _connmanager(manager), _th_count(n) {
            }

            ~RequestHandler() {
                for (std::vector<ThreadPtr>::iterator it = _threads.begin();
                     it != _threads.end(); ++it) {
                    if (*it) {
                        (*it)->cancel();
                        (*it)->join();
                    }
                }
            }

            void dispatcher(const request &req);

            void run() {
                for (int16 i = 0; i < _th_count; ++i) {
                    ThreadPtr ptr(new safe::Thread(std::bind(&RequestHandler::worker, this)));
                    _threads.push_back(ptr);
                    ptr->start();
                }
            }

        private:
            void handle_request(request &req);

            //handle request
            void worker() {
                LOG << "request handler worker running...";
                for (;;) {
                    request req;
                    _tasks.pop(req);
                    handle_request(req);
                }
            }


            Dispatcher &_epollcore;
            ConnManager &_connmanager;    //连接管理器
            std::vector<ThreadPtr> _threads;
            int32 _th_count;

            safe::block_queue<request> _tasks;


            DISALLOW_COPY_AND_ASSIGN(RequestHandler);
        };

        typedef std::shared_ptr<RequestHandler> RequestHandlerPtr;

    }//namespace ipc
}//namespace proxy


#endif