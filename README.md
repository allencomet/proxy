proxy_server
------------------------------------
a proxy server code that may help you learn network programming

Copyright 2008 r9wemeet@gmail.com Inc.

https://github.com/allencomet/proxy.git

Overview
--------
这是一个网络编程中经常会遇到的一些问题整合的代码集合，里面涉及到了多线程结合epoll利用ET模式如何实现高效实现网络通信的相关知识，
为了代码能够在一些老的gcc编译器上运行，本源代码使用了boost库去代替一些c++11的功能实现，希望这份源码可以帮助正在学习网络编程的
小伙伴，代码上如有误之处（本猿才疏学浅，写这份代码时大学毕业一年半，知识有限），望客观批评，将错误信息通过r9wemeet@gmail.com发
送到本人邮箱，如本人看到定会及时恢复并纠正。

usage
------------------------------------
```cpp
[root@localhost bin]# ./proxy -srvip=* -srvport=8080 &;tail -f log/proxy.INFO

[root@localhost bin]# ./rpc_client 127.0.0.1 8080

Note:
------------------------------------
由于程序中捕获了SIG_INT信号，所以如果./proxy -srvip=* -srvport=8080启动后以ctrl+c方式尝试停止程序运行是行不通的，
正确的做法应该是：
（1）找到该服务进程ID：ps -elf | grep proxy 
（2）然后kill pid


test:
------------------------------------
在bin目录下执行./test -a会启动一个代理服务器和一个客户端进行为期10秒的测试

(1)
测试接口：1000	动态创建一个后端新进程为该连接进行后续服务器（注意不是子进程，是detach后的子进程，即与当前的进程没有关系）
(2)
测试接口：2000  直接从代理服务器响应获取响应
(4)
测试接口：3000  直接从后端服务器通过IPC协议获取响应