#include "util/util.h"
#include "proxy/proxy.h"

/*
* usage:
*   ./exe -srvip=* -srvport=8080 -log2stderr
*   ./exe -srvpath=mastersrv -log2stderr
* or
*   ./exe --config=xx.conf
*
*/
int main(int argc, char** argv){
	ccflag::init_ccflag(argc, argv);
	cclog::init_cclog(os::get_process_name());

	//LOG << os::system("/root/proxy_server/debug/bin/bus/backend -srvpath=/root/proxy_server/debug/bin/bus/unixsock/allen &");
	//for (;;) sys::sleep(60);
	//单个epoll处理示例
	/*epollthreadpool::EpollCore netcore("*", ::FLG_srvport);
	netcore.init();
	netcore.run();*/

	//RPC: 多个epoll处理示例
	proxy::rpc::NetServer srv(::FLG_srvip, ::FLG_srvport);
	srv.run();

	////IPC: 多个epoll处理示例
	//epollthreadpool::LocalServer srv(::FLG_srvpath);
	//srv.run();

    return 0;
}



/*
在一个非阻塞的socket上调用read/write函数, 返回EAGAIN或者EWOULDBLOCK(注: EAGAIN就是EWOULDBLOCK)
从字面上看, 意思是:EAGAIN: 再试一次，EWOULDBLOCK: 如果这是一个阻塞socket, 操作将被block，perror输出: Resource temporarily unavailable

总结:
这个错误表示资源暂时不够，能read时，读缓冲区没有数据，或者write时，写缓冲区满了。遇到这种情况，如果是阻塞socket，
read/write就要阻塞掉。而如果是非阻塞socket，read/write立即返回-1， 同时errno设置为EAGAIN。
所以，对于阻塞socket，read/write返回-1代表网络出错了。但对于非阻塞socket，read/write返回-1不一定网络真的出错了。
可能是Resource temporarily unavailable。这时你应该再试，直到Resource available。

综上，对于non-blocking的socket，正确的读写操作为:
读：忽略掉errno = EAGAIN的错误，下次继续读
写：忽略掉errno = EAGAIN的错误，下次继续写

对于select和epoll的LT模式，这种读写方式是没有问题的。但对于epoll的ET模式，这种方式还有漏洞。


epoll的两种模式LT和ET
二者的差异在于level-trigger模式下只要某个socket处于readable/writable状态，无论什么时候进行epoll_wait都会返回该socket；
而edge-trigger模式下只有某个socket从unreadable变为readable或从unwritable变为writable时，epoll_wait才会返回该socket。

所以，在epoll的ET模式下，正确的读写方式为:
读：只要可读，就一直读，直到返回0，或者 errno = EAGAIN
写:只要可写，就一直写，直到数据发送完，或者 errno = EAGAIN

正确的读
n = 0;
while ((nread = read(fd, buf + n, BUFSIZ-1)) > 0) {
    n += nread;
}
if (nread == -1 && errno != EAGAIN) {
    perror("read error");
}
正确的写
int nwrite, data_size = strlen(buf);
n = data_size;
while (n > 0) {
    nwrite = write(fd, buf + data_size - n, n);
    if (nwrite < n) {
        if (nwrite == -1 && errno != EAGAIN) {
            perror("write error");
        }
        break;
    }
    n -= nwrite;
}
正确的accept，accept 要考虑 2 个问题
(1) 阻塞模式 accept 存在的问题
考虑这种情况：TCP连接被客户端夭折，即在服务器调用accept之前，客户端主动发送RST终止连接，导致刚刚建立的连接从就绪队列中移出，如果套接
口被设置成阻塞模式，服务器就会一直阻塞在accept调用上，直到其他某个客户建立一个新的连接为止。但是在此期间，服务器单纯地阻塞在accept调
用上，就绪队列中的其他描述符都得不到处理。

解决办法是把监听套接口设置为非阻塞，当客户在服务器调用accept之前中止某个连接时，accept调用可以立即返回-1，这时源自Berkeley的实现会在
内核中处理该事件，并不会将该事件通知给epool，而其他实现把errno设置为ECONNABORTED或者EPROTO错误，我们应该忽略这两个错误。

(2)ET模式下accept存在的问题
考虑这种情况：多个连接同时到达，服务器的TCP就绪队列瞬间积累多个就绪连接，由于是边缘触发模式，epoll只会通知一次，accept只处理一个连接，
导致TCP就绪队列中剩下的连接都得不到处理。

解决办法是用while循环抱住accept调用，处理完TCP就绪队列中的所有连接后再退出循环。如何知道是否处理完就绪队列中的所有连接呢？accept返回
-1并且errno设置为EAGAIN就表示所有连接都处理完。

综合以上两种情况，服务器应该使用非阻塞地accept，accept在ET模式下的正确使用方式为：
while ((conn_sock = accept(listenfd,(struct sockaddr *) &remote, (size_t *)&addrlen)) > 0) {
    handle_client(conn_sock);
}
if (conn_sock == -1) {
    if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
    perror("accept");
}
一道腾讯后台开发的面试题
使用Linuxepoll模型，水平触发模式；当socket可写时，会不停的触发socket可写的事件，如何处理？

第一种最普遍的方式：
需要向socket写数据的时候才把socket加入epoll，等待可写事件。
接受到可写事件后，调用write或者send发送数据。
当所有数据都写完后，把socket移出epoll。

这种方式的缺点是，即使发送很少的数据，也要把socket加入epoll，写完后在移出epoll，有一定操作代价。

一种改进的方式：
开始不把socket加入epoll，需要向socket写数据的时候，直接调用write或者send发送数据。如果返回EAGAIN，把socket加入epoll，在epoll的驱动下写数据，
全部数据发送完毕后，再移出epoll。

这种方式的优点是：数据不多的时候可以避免epoll的事件处理，提高效率。

最后贴一个使用epoll,ET模式的简单HTTP服务器代码:

*/