#ifndef __PROXY_RPC_REQUEST_HANDLER_H
#define __PROXY_RPC_REQUEST_HANDLER_H

#include "../../util/util.h"
#include "../core/protocol.h"

DEC_uint32(conntype);
DEC_string(remotepath);

namespace proxy {

    class ConnManager;

    class Connection;

    typedef std::shared_ptr<proxy::Connection> ConnectionPtr;

    namespace rpc {

        class Dispatcher;

        class RequestHandler {
        public:
            RequestHandler(Dispatcher &epollcore, ConnManager &front, ConnManager &back, int32 n = 4)
                    : _epollcore(epollcore), _conn_mng_front(front), _conn_mng_back(back), _th_count(n) {
            }

            ~RequestHandler() {
                for (auto it = _threads.begin();
                     it != _threads.end(); ++it) {
                    if (*it) {
                        (*it)->cancel();
                        (*it)->join();
                    }
                }
            }

            void dispatcher(const request &req);

            void back_reply_to_front(const request &req);

            void run() {
                for (int16 i = 0; i < _th_count; ++i) {
                    ThreadPtr ptr(new safe::Thread(std::bind(&RequestHandler::worker, this)));
                    _threads.push_back(ptr);
                    ptr->start();
                }
            }

            void inform_back_exit();

        private:
            void create_new_back_process(const request &req);

            void handle_2back_request(request &req);
            void handle_ipc_request(const request &req);

            void handle_rpc_request(const request &req);

            void handle_inter_request(const request &req);

            void handle_error_request(const request &req, int32 err, const std::string &msg);

            void handle_unknown_request(const request &req);

            void handle_illegal_request(const request &req);


            void front_request_to_back(ConnectionPtr backptr, const request &req,
                                       const std::string &userid = "");

            inline void save_back_path(const std::string &path) {
                safe::MutexGuard g(_back_path_mtx);
                _back_path.insert(path);
                LOG << "insert " << path << " to set container";
            }

            bool aux_inform_to_back(const std::string &, IPCPACK &);

            // handle request
            void worker() {
                LOG << "request handler worker running...";
                for (;;) {
                    request req;
                    _tasks.pop(req);
                    handle_2back_request(req);
                }
            }


            Dispatcher &_epollcore;
            ConnManager &_conn_mng_front;    // a manager that processes front-end input requests
            ConnManager &_conn_mng_back;     // a manager that processes back-end input requests
            std::vector<ThreadPtr> _threads;
            int32 _th_count;

            safe::Mutex _back_path_mtx;
            std::set<std::string> _back_path;

            safe::block_queue<request> _tasks;

            DISALLOW_COPY_AND_ASSIGN(RequestHandler);
        };

        typedef std::shared_ptr<RequestHandler> RequestHandlerPtr;

    }//namespace rpc
}//namespace proxy


#endif