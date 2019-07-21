#include "dispatcher.h"

#include "../core/parser.h"

namespace proxy {
    namespace rpc {

        void Dispatcher::init(int32 listenfd) {
            _listenfd = listenfd;

            if (!os::path.exists("./bus/unixsock")) os::makedirs("./bus/unixsock");

            // create epoll for front events
            _epfd_front = ::epoll_create(_event_limit);
            if (_epfd_front == -1) {
                FLOG << "epoll_create error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            // create epoll for back events
            _epfd_back = ::epoll_create(_event_limit);
            if (_epfd_back == -1) {
                FLOG << "epoll_create error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            // register to listen for client events
            _ev_front.events = EPOLLIN;
            _ev_front.data.fd = _listenfd;
            if (::epoll_ctl(_epfd_front, EPOLL_CTL_ADD, _listenfd, &_ev_front) == -1) {
                FLOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            // initialize handler to handle requests
            _request_handler.reset(new RequestHandler(*this, _connmanager_front, _connmanager_back));
            _request_handler->run();

            // create thread to handle front requests
            _front_threadptr.reset(new safe::Thread(std::bind(&Dispatcher::run_front, this)));
            // create thread to handle back responses
            _back_threadptr.reset(new safe::Thread(std::bind(&Dispatcher::run_back, this)));
        }

        // Linux2.6版本之前还存在对于socket的accept的惊群现象,之后的版本已经解决掉了这个问题
        // 惊群是指多个进程/线程在等待同一资源时，每当资源可用，所有的进程/线程都来竞争资源的现象
        void Dispatcher::init() {
            _listenfd = net::tcp_socket();

            if (!os::path.exists("./bus/unixsock")) os::makedirs("./bus/unixsock");

            net::setnonblocking(_listenfd);
            int32 optionVal = 1;
            ::setsockopt(_listenfd, SOL_SOCKET, SO_REUSEADDR, &optionVal, sizeof(optionVal));

            net::ipv4_addr localaddr(_ip, _port);
            net::bind(_listenfd, localaddr);
            net::listen(_listenfd, _listen_limit);

            // create epoll for back events
            _epfd_front = ::epoll_create(_event_limit);
            if (_epfd_front == -1) {
                FLOG << "epoll_create error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            // create epoll for back events
            _epfd_back = ::epoll_create(_event_limit);
            if (_epfd_back == -1) {
                FLOG << "epoll_create error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            // register to listen for client events
            _ev_front.events = EPOLLIN;
            _ev_front.data.fd = _listenfd;
            if (::epoll_ctl(_epfd_front, EPOLL_CTL_ADD, _listenfd, &_ev_front) == -1) {
                FLOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            _request_handler.reset(new RequestHandler(*this, _connmanager_front, _connmanager_back));
            _request_handler->run();

            // create thread to handle front requests
            _front_threadptr.reset(new safe::Thread(std::bind(&Dispatcher::run_front, this)));
            // create thread to handle back responses
            _back_threadptr.reset(new safe::Thread(std::bind(&Dispatcher::run_back, this)));
        }


        bool Dispatcher::handle_request(ConnectionPtr ptr, bool is_front) {
            bool do_next = true;
            for (;;) {
                request req;
                req.fd = ptr->fd();
                bool stop_parse = false;
                switch (ParseTool::parse(ptr->rbuf(), req)) {
                    case ParseTool::kParseError:
                        if (is_front) {
                            close_client_front(ptr->fd());  // marked as stoppable
                        } else {
                            close_client_back(ptr->fd());   // marked as stoppable
                        }
                        stop_parse = true;      // parse error, stop handling next
                        do_next = false;        // stop handling
                        WLOG << "parser error";
                        break;
                    case ParseTool::kParseInProgress:
                        // buffer data is not enough for a complete package
                        stop_parse = true;
                        break;
                    case ParseTool::kParseHasDone:
                        // puts request was parsed into queue, and handle next
                        _request_handler->dispatcher(req);
                        stop_parse = true;  // there is no buffer data needs to parse
                        break;
                    case ParseTool::kParseHasLeft:
                        // puts request was parsed into queue, and handle next
                        _request_handler->dispatcher(req);
                        break;
                    default:    // unknown request,close connection
                        if (is_front) {
                            close_client_front(ptr->fd());  // marked as stoppable
                        } else {
                            close_client_back(ptr->fd());   // marked as stoppable
                        }
                        break;
                }

                if (stop_parse) break;
            }
            return do_next;
        }

        bool Dispatcher::handle_request_from_front(ConnectionPtr ptr) {
            return handle_request(ptr, true);
        }

        bool Dispatcher::handle_response_from_back(ConnectionPtr ptr) {
            return handle_request(ptr, true);
        }

        void Dispatcher::handle_accept_event(int32 fd) {
            //用while循环抱住accept调用，处理完TCP就绪队列中的所有连接后再退出循环,
            //accept返回-1并且errno设置为EAGAIN就表示所有连接都处理完
            int32 conn_sock;
            net::ipv4_addr remoteaddr;
            while ((conn_sock = net::accept(fd, &remoteaddr)) > 0) {
                net::setnonblocking(conn_sock);  // setting noblock
                init_socket_option(conn_sock);   // setting other options

                // add new connection
                if (!_connmanager_front.add_conn(conn_sock, remoteaddr.to_string())) {
                    server_busy(conn_sock);
                    return;
                }

                LOG << "accept new client: fd[" << conn_sock << "],addr[" << remoteaddr.to_string() << "]";
                register_add_read_event_front(conn_sock);
            }

            if (conn_sock == -1) {
                if (errno != EAGAIN && errno != ECONNABORTED
                    && errno != EPROTO && errno != EINTR) {
                    ELOG << "accept error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
                    return;
                }
                LOG << "has accepted all clients";
            }
        }

        void Dispatcher::handle_read_event_front(int32 fd) {
            // in edge-triggered mode events are only triggered once, so all data needs to be read at once
            int32 nread, n = 0;

            while ((nread = ::read(fd, _read_tmp_buf_front + n, MAXLINE - 1)) > 0) {
                n += nread;
            }

            if (nread == -1 && errno != EAGAIN) {
                ELOG << "read error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
                close_client_front(fd);
            } else if (nread == 0) {
                WLOG << "closed by peer(" << fd << "): [" << errno << ":" << ::strerror(errno) << "]";
                close_client_front(fd);
            } else {
                ConnectionPtr connptr = _connmanager_front.connection(fd);
                if (!connptr) {
                    close_client_front(fd);
                    return;
                }

                LOG << "read " << n << " bytes data from client(" << fd << ":" << connptr->fd() << ")";
                connptr->set_msg_time(sys::utc.ms());
                connptr->append_rbuf_ns(_read_tmp_buf_front, n);
                handle_request_from_front(connptr);
            }
        }

        void Dispatcher::handle_read_event_back(int32 fd) {
            // in edge-triggered mode events are only triggered once, so all data needs to be read at once
            int32 nread, n = 0;

            while ((nread = ::read(fd, _read_tmp_buf_back + n, MAXLINE - 1)) > 0) {
                n += nread;
            }

            if (nread == -1 && errno != EAGAIN) {
                ELOG << "read error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
                close_client_back(fd);
            } else if (nread == 0) {
                WLOG << "closed by peer(" << fd << "): [" << errno << ":" << ::strerror(errno) << "]";
                close_client_back(fd);
            } else {
                ConnectionPtr connptr = _connmanager_back.connection(fd);
                if (!connptr) {
                    close_client_back(fd);
                    return;
                }

                LOG << "read " << n << " bytes data from backend(" << fd << ":" << connptr->fd() << ")";
                connptr->set_msg_time(sys::utc.ms());
                connptr->append_rbuf_ns(_read_tmp_buf_back, n);
                handle_response_from_back(connptr);
            }
        }

        void Dispatcher::handle_write_event_front(int32 fd) {
            ConnectionPtr connptr = _connmanager_front.connection(fd);
            if (!connptr) {
                ELOG << "can't find the specify client(" << fd << ")";
                close_client_front(fd);
                return;
            }

            std::deque<StreamBufPtr> reply_queue;
            connptr->get_write_pack_s(reply_queue);
            for (auto it = reply_queue.begin(); it != reply_queue.end(); ++it) {
                if (-1 == net::writen(fd, (*it)->data(), (*it)->size())) {
                    close_client_front(fd);
                    break;
                }
            }
        }

        void Dispatcher::handle_write_event_back(int32 fd) {
            ConnectionPtr connptr = _connmanager_back.connection(fd);
            if (!connptr) {
                return;
            }

            std::deque<StreamBufPtr> write_queue;
            connptr->get_write_pack_s(write_queue);
            LOG << "get " << write_queue.size() << " packets to send to backend[" << fd << "]";
            for (auto it = write_queue.begin(); it != write_queue.end(); ++it) {
                if (-1 == net::writen(fd, (*it)->data(), (*it)->size())) {
                    close_client_back(fd);
                    break;
                }
                IPCPACK *tmp = (IPCPACK *) (*it)->data();
                LOG << "epoll has written a packet to back,information: userid[" << tmp->ueser_id << "],bodylen["
                    << tmp->body_len
                    << "],content[" << (*it)->data() + g_kIPCLen << "],size:[" << (*it)->size() - g_kIPCLen << "]";
            }
        }

        void Dispatcher::close_client_front(int32 fd) {
            _ev_front.data.fd = fd;
            if (epoll_ctl(_epfd_front, EPOLL_CTL_DEL, fd, &_ev_front) == -1) {
                ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }
            ::close(fd);
            _connmanager_front.mark_death(fd);

            _connmapptr->dissociate_by_front(fd);
        }

        void Dispatcher::close_client_back(int32 fd) {
            _ev_back.data.fd = fd;
            if (epoll_ctl(_epfd_back, EPOLL_CTL_DEL, fd, &_ev_front) == -1) {
                ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }
            ::close(fd);
            _connmanager_back.mark_death(fd);

            _connmapptr->dissociate_by_back(fd);
        }

        void Dispatcher::server_busy(int32 fd) {
            WLOG << "server busy...";
            close_client_front(fd);
        }

        void Dispatcher::notify_back_exit() {
            if (_request_handler) {
                _request_handler->inform_back_exit();
            } else {
                WLOG << "request_handler is null";
            }
            WLOG << "wait for backend stop running...";

            //sys::msleep(1000);
        }

        void Dispatcher::init_socket_option(int32 fd) {
            int nRecvBuf = 128 * 1024;
            ::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *) &nRecvBuf, sizeof(int));
            // setting send buffer
            int32 nSendBuf = 128 * 1024;
            ::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *) &nSendBuf, sizeof(int));

            int32 nNetTimeout = 1000;   // 1 second
            // setting send time out
            ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *) &nNetTimeout, sizeof(int));

            // setting receive time out
            ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &nNetTimeout, sizeof(int));
        }

        void Dispatcher::run_front() {
            int nfds, fd, i;
            for (;;) {
                // check and remove invalid connections
                _connmanager_front.check_invalid_conn();

                int32 nfds = ::epoll_wait(_epfd_front, _events_front, MAX_EVENTS, 100);
                if (nfds == -1) {
                    WLOG << "epoll_wait error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
                    continue;
                }

                for (i = 0; i < nfds; ++i) {
                    fd = _events_front[i].data.fd;
                    if (fd == _listenfd) {
                        handle_accept_event(fd);
                    } else if (_events_front[i].events & EPOLLIN) {
                        handle_read_event_front(fd);
                    } else if (_events_front[i].events & EPOLLOUT) {
                        handle_write_event_front(fd);
                    } else if (_events_front[i].events & EPOLLHUP) {
                        WLOG << "exception occurred(" << fd << "): [" << errno << ":" << ::strerror(errno) << "]";
                        close_client_front(fd);
                    }
                }//end for
            }//end for(;;)
        }

        void Dispatcher::run_back() {
            int nfds, fd, i;
            for (;;) {
                // check and remove invalid connections
                _connmanager_back.check_invalid_conn();

                int32 nfds = ::epoll_wait(_epfd_back, _events_back, MAX_EVENTS, 100);
                if (nfds == -1) {
                    WLOG << "epoll_wait error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
                    continue;
                }

                for (i = 0; i < nfds; ++i) {
                    fd = _events_back[i].data.fd;
                    if (_events_back[i].events & EPOLLIN) {
                        handle_read_event_back(fd);
                    } else if (_events_back[i].events & EPOLLOUT) {
                        handle_write_event_back(fd);    // temporarily unavailable
                    } else if (_events_back[i].events & EPOLLHUP) {
                        WLOG << "exception occurred(" << fd << "): [" << errno << ":" << ::strerror(errno) << "]";
                        close_client_back(fd);
                    }
                }//end for
            }//end for(;;)
        }

        void Dispatcher::run() {
            LOG << "server launch...";
            if (_front_threadptr) _front_threadptr->start();
            if (_back_threadptr) _back_threadptr->start();

            for (;;) {
                sys::sleep(60);
            }
        }

    }//namespace rpc
}//namespace proxy