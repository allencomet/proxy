#ifndef __PROXY_IPC_H
#define __PROXY_IPC_H

#include "../../util/util.h"

DEC_uint32(thcount);        //the number of thread in server
DEC_string(srvpath);        //ipc server bind address
DEC_string(srvip);            //rpc server bind address
DEC_uint32(srvport);        //server listen port

namespace proxy {
    namespace ipc {

        class LocalServer {
        public:
            typedef std::shared_ptr<safe::Thread> ThreadPtr;

            explicit LocalServer(const std::string &path);

            ~LocalServer() {
                stop();
            }

            void run();

            void stop() {
                for (auto it = _threads.begin();
                     it != _threads.end(); ++it) {
                    if (*it) {
                        (*it)->cancel();
                        (*it)->join();
                    }
                }
            }

        private:
            void worker(int32 listenfd);

            std::string _path;
            int32 _listenfd;
            std::vector<ThreadPtr> _threads;

            DISALLOW_COPY_AND_ASSIGN(LocalServer);
        };


    }//namespace ipc
}//namespace proxy

#endif