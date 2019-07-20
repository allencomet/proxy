#include "dispatcher_rpc.h"

#include "../core/parser.h"

namespace proxy {
    namespace rpc {
        int32 Dispatcher::init_tcp_socket(const std::string &ip, const int16 &port) {
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

        void Dispatcher::init(int32 listenfd) {
            _listenfd = listenfd;

            if (!os::path.exists("./bus/unixsock")) os::makedirs("./bus/unixsock");
            //if (!os::path.exists("/tmp/ctp_proxy_server")) os::makedirs("/tmp/ctp_proxy_server");

            //创建基于前端客户的事件反应堆
            _epfd_front = ::epoll_create(_event_limit);
            if (_epfd_front == -1) {
                FLOG << "epoll_create error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            //创建基于与后端通信的事件反应堆
            _epfd_back = ::epoll_create(_event_limit);
            if (_epfd_back == -1) {
                FLOG << "epoll_create error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            //将监听描述符注册到前端反应堆
            _ev_front.events = EPOLLIN;
            _ev_front.data.fd = _listenfd;
            if (::epoll_ctl(_epfd_front, EPOLL_CTL_ADD, _listenfd, &_ev_front) == -1) {
                FLOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            _request_handler.reset(new RequestHandler(*this, _connmanager_front, _connmanager_back));
            _request_handler->run();

            _front_threadptr.reset(new safe::Thread(std::bind(&Dispatcher::run_front, this)));
            _back_threadptr.reset(new safe::Thread(std::bind(&Dispatcher::run_back, this)));
        }

        //Linux2.6版本之前还存在对于socket的accept的惊群现象,之后的版本已经解决掉了这个问题
        //惊群是指多个进程/线程在等待同一资源时，每当资源可用，所有的进程/线程都来竞争资源的现象
        void Dispatcher::init() {
            _listenfd = net::tcp_socket();

            if (!os::path.exists("./bus/unixsock")) os::makedirs("./bus/unixsock");
            //if (!os::path.exists("/tmp/ctp_proxy_server")) os::makedirs("/tmp/ctp_proxy_server");

            net::setnonblocking(_listenfd);
            int32 optionVal = 1;
            ::setsockopt(_listenfd, SOL_SOCKET, SO_REUSEADDR, &optionVal, sizeof(optionVal));

            net::ipv4_addr localaddr(_ip, _port);
            net::bind(_listenfd, localaddr);
            net::listen(_listenfd, _listen_limit);

            //创建基于前端客户的事件反应堆
            _epfd_front = ::epoll_create(_event_limit);
            if (_epfd_front == -1) {
                FLOG << "epoll_create error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            //创建基于与后端通信的事件反应堆
            _epfd_back = ::epoll_create(_event_limit);
            if (_epfd_back == -1) {
                FLOG << "epoll_create error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            //将监听描述符注册到前端反应堆
            _ev_front.events = EPOLLIN;
            _ev_front.data.fd = _listenfd;
            if (::epoll_ctl(_epfd_front, EPOLL_CTL_ADD, _listenfd, &_ev_front) == -1) {
                FLOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }

            _request_handler.reset(new RequestHandler(*this, _connmanager_front, _connmanager_back));
            _request_handler->run();

            _front_threadptr.reset(new safe::Thread(std::bind(&Dispatcher::run_front, this)));
            _back_threadptr.reset(new safe::Thread(std::bind(&Dispatcher::run_back, this)));
        }

        bool Dispatcher::handle_request_from_front(ConnectionPtr ptr) {

            bool status = true;
            for (;;) {
                request req;
                req.fd = ptr->fd();
                bool flag = false;
                switch (PaserTool::paser(ptr->rbuf(), req)) {
                    case PaserTool::kPaserError:
                        close_client_front(ptr->fd());//从epoll中移除该描述符(标记为等待关闭的连接)
                        flag = true;//解析请求包出错，跳出for循环
                        status = false;//表示出错，外层不必继续往下处理
                        ELOG << "parser error";
                        break;
                    case PaserTool::kPaserInprogress:
                        flag = true;//缓冲区内数据不足一个包，跳出for循环继续接收数据
                        break;
                    case PaserTool::kPaserHasdone:
                        _request_handler->dispatcher(req);//将请求包放到请求队列
                        flag = true;//已经处理完所有请求，跳出for循环继续接收数据
                        break;
                    case PaserTool::kPaserHasleft:
                        _request_handler->dispatcher(req);//将请求包放到请求队列
                        break;
                    default://unknow request,close connection
                        close_client_front(ptr->fd());//从epoll中移除该描述符(标记为等待关闭的连接)
                        break;
                }

                if (flag) break;//跳出for循环
            }
            return status;
        }

        //将消息传输给关联的前端客户
        bool Dispatcher::handle_reply_from_back(ConnectionPtr ptr) {
            bool status = true;
            for (;;) {
                request req;
                req.fd = ptr->fd();
                bool flag = false;
                switch (PaserTool::paser(ptr->rbuf(), req)) {
                    case PaserTool::kPaserError:
                        close_client_back(ptr->fd());//从epoll中移除该描述符(标记为等待关闭的连接)
                        flag = true;//解析请求包出错，跳出for循环
                        status = false;//表示出错，外层不必继续往下处理
                        ELOG << "parser error";
                        break;
                    case PaserTool::kPaserInprogress:
                        flag = true;//缓冲区内数据不足一个包，跳出for循环继续接收数据
                        break;
                    case PaserTool::kPaserHasdone:
                        _request_handler->dispatcher(req);//将请求包放到请求队列
                        flag = true;//已经处理完所有请求，跳出for循环继续接收数据
                        break;
                    case PaserTool::kPaserHasleft:
                        _request_handler->dispatcher(req);//将请求包放到请求队列
                        break;
                    default://unknow request,close connection
                        close_client_back(ptr->fd());//从epoll中移除该描述符(标记为等待关闭的连接)
                        break;
                }

                if (flag) break;//跳出for循环
            }
            return status;
        }

        void Dispatcher::handle_accept_event(int32 fd) {
            //用while循环抱住accept调用，处理完TCP就绪队列中的所有连接后再退出循环,
            //accept返回-1并且errno设置为EAGAIN就表示所有连接都处理完
            int32 conn_sock;
            net::ipv4_addr remoteaddr;
            while ((conn_sock = net::accept(fd, &remoteaddr)) > 0) { //接受客户连接并返回已连接套接字描述符
                net::setnonblocking(conn_sock);  //设置套接字描述符为非阻塞的
                init_socket_option(conn_sock);    //设置套接字选项
                if (!_connmanager_front.add_conn(conn_sock, remoteaddr.to_string())) {//新建客户
                    server_busy(conn_sock);
                    return;
                } else {
                    LOG << "accept new client: fd[" << conn_sock << "],addr[" << remoteaddr.to_string() << "]";
                    register_add_read_event_front(conn_sock);
                }
            }

            if (conn_sock == -1) {
                if (errno != EAGAIN && errno != ECONNABORTED
                    && errno != EPROTO && errno != EINTR) {
                    ELOG << "accept error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
                } else {
                    LOG << "has accepted all clients";
                }
            }
        }

        void Dispatcher::handle_read_event_front(int32 fd) {
            //边缘触发模式下事件只会被触发一次，所以需要一次性把所有数据读取完
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
                if (connptr) {
                    LOG << "read " << n << " bytes data from client(" << fd << ":" << connptr->fd() << ")";
                    connptr->set_msg_time(sys::utc.ms());
                    connptr->append_rbuf_ns(_read_tmp_buf_front, n);
                    handle_request_from_front(connptr);//处理请求包成功之后原来读事件上新增写事件（异步处理，在有数据响应的时候增加写事件）
                } else {
                    close_client_front(fd);
                }
            }
        }

        void Dispatcher::handle_read_event_back(int32 fd) {
            //边缘触发模式下事件只会被触发一次，所以需要一次性把所有数据读取完
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
                if (connptr) {
                    LOG << "read " << n << " bytes data from backend(" << fd << ":" << connptr->fd() << ")";
                    connptr->set_msg_time(sys::utc.ms());
                    connptr->append_rbuf_ns(_read_tmp_buf_back, n);
                    handle_reply_from_back(connptr);//处理后端响应的消息
                } else {
                    close_client_back(fd);
                }
            }
        }

        void Dispatcher::handle_write_event_front(int32 fd) {
            ConnectionPtr connptr = _connmanager_front.connection(fd);
            if (connptr) {
                std::deque<StreamBufPtr> reply_queue;
                connptr->get_write_pack_s(reply_queue);//从用户响应队列中取出
                for (std::deque<StreamBufPtr>::iterator it = reply_queue.begin();
                     it != reply_queue.end(); ++it) {
                    if (-1 == net::writen(fd, (*it)->data(), (*it)->size())) {
                        close_client_front(fd);//从epoll中移除该描述符
                        break;
                    }
                }
            } else {
                ELOG << "can't find the specify client(" << fd << ")";
                close_client_front(fd);
            }
        }

        void Dispatcher::handle_write_event_back(int32 fd) {
            ConnectionPtr connptr = _connmanager_back.connection(fd);
            if (connptr) {
                std::deque<StreamBufPtr> write_queue;
                connptr->get_write_pack_s(write_queue);//从发送队列中取出
                LOG << "get " << write_queue.size() << " packets to send to backend[" << fd << "]";
                for (std::deque<StreamBufPtr>::iterator it = write_queue.begin();
                     it != write_queue.end(); ++it) {
                    if (-1 == net::writen(fd, (*it)->data(), (*it)->size())) {
                        close_client_back(fd);//从epoll中移除该描述符
                        break;
                    }
                    IPCPACK *tmp = (IPCPACK *) (*it)->data();
                    LOG << "epoll has written a packet to back,information: userid[" << tmp->ueser_id << "],bodylen["
                        << tmp->body_len
                        << "],content[" << (*it)->data() + g_kIPCLen << "],size:[" << (*it)->size() - g_kIPCLen << "]";
                }
            }
        }

        void Dispatcher::close_client_front(int32 fd) {
            _ev_front.data.fd = fd;
            if (epoll_ctl(_epfd_front, EPOLL_CTL_DEL, fd, &_ev_front) == -1) {
                ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }
            ::close(fd);
            _connmanager_front.mark_death(fd);//标记为等待关闭的连接

            _connmapptr->unassociate_by_front(fd);//解除前后端的关联
        }

        void Dispatcher::close_client_back(int32 fd) {
            _ev_back.data.fd = fd;
            if (epoll_ctl(_epfd_back, EPOLL_CTL_DEL, fd, &_ev_front) == -1) {
                ELOG << "epoll_ctl error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
            }
            ::close(fd);
            _connmanager_back.mark_death(fd);//标记为等待关闭的连接

            _connmapptr->unassociate_by_back(fd);//解除前后端的关联
        }

        void Dispatcher::server_busy(int32 fd) {
            WLOG << "server busy...";
            close_client_front(fd);
        }

        void Dispatcher::inform_back_exit() {
            if (_request_handler) {
                _request_handler->inform_back_exit();
            } else {
                WLOG << "request_handler is null";
            }
            WLOG << "wait for backend stop running...";

            //sys::msleep(1000);
        }

        void Dispatcher::init_socket_option(int32 fd) {
            int nRecvBuf = 128 * 1024;//设置为32K
            ::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *) &nRecvBuf, sizeof(int));
            //发送缓冲区
            int32 nSendBuf = 128 * 1024;//设置为32K
            ::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *) &nSendBuf, sizeof(int));

            int32 nNetTimeout = 1000;//1秒
            //发送时限
            ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *) &nNetTimeout, sizeof(int));

            //接收时限
            ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &nNetTimeout, sizeof(int));
        }

        void Dispatcher::run_front() {
            int nfds, fd, i;
            for (;;) {
                //在这里负责连接的删除
                _connmanager_front.check_invalid_conn();

                //等待描述符准备就绪，返回就绪数量
                int32 nfds = ::epoll_wait(_epfd_front, _events_front, MAX_EVENTS, 100);
                if (nfds == -1) {
                    ELOG << "epoll_wait error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
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
                        ELOG << "exception occurred(" << fd << "): [" << errno << ":" << ::strerror(errno) << "]";
                        close_client_front(fd);
                    }
                }//end for
            }//end for(;;)
        }

        void Dispatcher::run_back() {
            int nfds, fd, i;
            for (;;) {
                //在这里负责连接的删除
                _connmanager_back.check_invalid_conn();

                //等待描述符准备就绪，返回就绪数量
                int32 nfds = ::epoll_wait(_epfd_back, _events_back, MAX_EVENTS, 100);
                if (nfds == -1) {
                    ELOG << "epoll_wait error has been occurred: [" << errno << ":" << ::strerror(errno) << "]";
                    continue;
                }

                for (i = 0; i < nfds; ++i) {
                    fd = _events_back[i].data.fd;
                    if (_events_back[i].events & EPOLLIN) {
                        handle_read_event_back(fd);
                    } else if (_events_back[i].events & EPOLLOUT) {
                        handle_write_event_back(fd);//temporarily unavaiable
                    } else if (_events_back[i].events & EPOLLHUP) {
                        ELOG << "exception occurred(" << fd << "): [" << errno << ":" << ::strerror(errno) << "]";
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