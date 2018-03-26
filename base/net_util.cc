#include "net_util.h"

namespace net {

bool connect(int sockfd, const sock_addr& server) {
    int ret = ::connect(sockfd, server.addr(), server.size());
    if (ret == 0) return true;

	ELOG << "connect error: " << strerror(errno) << " sockfd: " << sockfd
		<< ", server: " << server.to_string();

    close(sockfd);
    return false;
}

bool bind(int sockfd, const sock_addr& addr) {
    int ret = ::bind(sockfd, addr.addr(), addr.size());
    if (ret == 0) return true;

	ELOG << "bind error: " << strerror(errno) << " sockfd: " << sockfd
		<< ", addr: " << addr.to_string();

    ::close(sockfd);
    return false;
}

bool listen(int sockfd, int backlog) {
    int ret = ::listen(sockfd, backlog);
    if (ret == 0) return true;

	ELOG << "listen error: " << ::strerror(errno) << " sockfd: " << sockfd;

    ::close(sockfd);
    return false;
}

bool setnonblocking(int sockfd) {
	int opts = ::fcntl(sockfd, F_GETFL);
	if (opts < 0) return false;
	opts = (opts | O_NONBLOCK);
	if (::fcntl(sockfd, F_SETFL, opts) < 0) return false;
	return true;
}

int accept(int sockfd, sock_addr* cli) {
    socklen_t len = cli->size();
    int fd = ::accept(sockfd, cli->addr(), &len);

    if (fd != -1) {
        CHECK_EQ(len, cli->size());
        return fd;
    }

	ELOG << "accept error: " << ::strerror(errno) << " sockfd: " << sockfd;
    return -1;
}

int unixaccept(int sockfd, unix_addr &cli){
	socklen_t len = cli.size();
	int fd = ::accept(sockfd, (struct sockaddr *)cli.addr(), &len);

	if (fd != -1) {
		CHECK_EQ(len, cli.size());
		return fd;
	}

	ELOG << "accept error: " << strerror(errno) << " sockfd: " << sockfd;
	return -1;
}

int32 read(int fd, void* ptr, uint32 n) {
    for(;;) {
        int r = ::read(fd, ptr, n);
        if (r >= 0) return r;

		if (errno == EAGAIN || errno == EINTR) {//EWOULDBLOCK其实就是EAGAIN
			continue;
		}

		ELOG << "read error: " << strerror(errno) << ", fd: " << fd;
        return -1;
    }
}

ssize_t readn(int fd, void *vptr, size_t n){
	ssize_t nread;
	size_t  nleft = n;
	char    *bp = (char *)vptr;

	while (nleft > 0){
		nread = ::read(fd, bp, nleft);
		if (nread < 0){
			if (errno == EINTR || errno == EAGAIN) {//EWOULDBLOCK其实就是EAGAIN
				nread = 0;
				break;
			} else {
				return -1;
			} 
		}else if (nread == 0) {
			break;
		}
			
		nleft -= nread;
		bp += nread;
	}
	return (n - nleft);
}

int32 write(int fd, const void* ptr, uint32 n) {
    for(;;) {
        size_t r = ::write(fd, ptr, n);
        CHECK_NE(r, 0);
        if (r > 0) return r;

		if (r < 0 && errno == EAGAIN || errno == EINTR) {//EWOULDBLOCK其实就是EAGAIN
			continue;
		}
		
		ELOG << "write error: " << strerror(errno) << ", fd: " << fd;
        return -1;
    }
}

ssize_t writen(int fd,const void *vptr,size_t n){
	size_t nleft;	//剩余量
	ssize_t nwritten;	//已写入量
	const char *ptr = (const char *)vptr;

	nleft=n;
	while(nleft>0){
		if( (nwritten=::write(fd,ptr,nleft)) <=0 ){
			//如果是因为系统捕捉信号和中断,或者是非阻塞套接字描述符,那么write重试
			if( nwritten < 0 && (errno == EINTR || errno == EAGAIN))//EWOULDBLOCK其实就是EAGAIN
				nwritten = 0;
			else
				return -1;
		}

		nleft-=nwritten;
		ptr+=nwritten;
	}

	return n;
}

bool getsockname(int fd, sock_addr* addr) {
    socklen_t len = addr->size();
    int ret = ::getsockname(fd, addr->addr(), &len);
    if (ret == 0) {
        CHECK_EQ(addr->size(), len);
        return true;
    }

	ELOG << "getsockname error: " << strerror(errno) << ", fd: " << fd;
    return false;
}

bool getpeername(int fd, sock_addr* addr) {
    socklen_t len = addr->size();
    int ret = ::getpeername(fd, addr->addr(), &len);
    if (ret == 0) {
        CHECK_EQ(addr->size(), len);
        return true;
    }

	ELOG << "getpeername error: " << strerror(errno) << ", fd: " << fd;
    return false;
}

std::string ip_to_string(uint32 ip) {
    struct in_addr addr;
    addr.s_addr = ip;

    char s[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &addr, s, sizeof(s));

    return std::string(s);
}

uint32 string_to_ip(const std::string& ip) {
    struct in_addr addr;
    int ret = inet_pton(AF_INET, ip.c_str(), &addr);
	CHECK_EQ(ret, 1) << "invalid ip: " << ip;

    return addr.s_addr;
}

std::vector<std::string> get_ip_by_name(const std::string& name) {
    std::vector<std::string> ips;

    struct addrinfo hints;
    ::memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* addr_info = NULL;
    int err = getaddrinfo(name.c_str(), NULL, &hints, &addr_info);

    if (err != 0) {
		ELOG << "getaddrinfo error, domain: " << name << ", err: "
			<< gai_strerror(err);
        return ips;
    }

    if (addr_info == NULL) return ips;

    for (struct addrinfo* it = addr_info; it != NULL; it = it->ai_next) {
        sockaddr_in* addr = (sockaddr_in*) it->ai_addr;
        ips.push_back(net::ip_to_string(addr->sin_addr.s_addr));
    }

    freeaddrinfo(addr_info);
    return ips;
}

} // namespace net
