#pragma once

#include "data_types.h"
#include "string_util.h"
#include "cclog/cclog.h"

#include <iostream>

#include <string.h>
#include <errno.h>
#include <endian.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace net {

inline uint16 hton16(uint16 v) {
    return htobe16(v);
}

inline uint32 hton32(uint32 v) {
    return htobe32(v);
}

inline uint64 hton64(uint64 v) {
    return htobe64(v);
}

inline uint16 ntoh16(uint16 v) {
    return be16toh(v);
}

inline uint32 ntoh32(uint32 v) {
    return be32toh(v);
}

inline uint64 ntoh64(uint64 v) {
    return be64toh(v);
}

/*
 * ip: big endian in network byte order
 */
std::string ip_to_string(uint32 ip);

uint32 string_to_ip(const std::string& ip);

class sock_addr {
  public:
	  sock_addr() {};
	  virtual ~sock_addr() {};

    virtual const struct sockaddr* addr() const = 0;
    virtual struct sockaddr* addr() = 0;

    virtual uint32 size() const = 0;

    virtual std::string to_string() const = 0;

private:
    DISALLOW_COPY_AND_ASSIGN(sock_addr);	//since c++11
};

class ipv4_addr : public sock_addr {
  public:
    ipv4_addr() {
        ::memset(&_addr, 0, sizeof(_addr));
    }

    /*
     * if ip is empty or ip == "*", set s_addr to INADDR_ANY
     */
    ipv4_addr(const std::string& ip, uint32 port){
		::memset(&_addr, 0, sizeof(_addr));
        _addr.sin_family = AF_INET;
        _addr.sin_port = net::hton16(static_cast<uint16>(port));

        if (!ip.empty() && ip != "*") {
            _addr.sin_addr.s_addr = net::string_to_ip(ip);
		}
    }

	virtual ~ipv4_addr() {}

    virtual const struct sockaddr* addr() const {
        return (struct sockaddr*) &_addr;
    }

    virtual struct sockaddr* addr() {
        return (struct sockaddr*) &_addr;
    }

    virtual uint32 size() const {
        return sizeof(_addr);
    }

    virtual std::string to_string() const {
        return net::ip_to_string(_addr.sin_addr.s_addr) + ":" +
               str::to_string(net::ntoh16(_addr.sin_port));
    }

  private:
    struct sockaddr_in _addr;

    DISALLOW_COPY_AND_ASSIGN(ipv4_addr);	//since c++11
};

class ipv6_addr : public sock_addr {
  public:
    ipv6_addr() {
        ::memset(&_addr, 0, sizeof(_addr));
    }

    ipv6_addr(const std::string& ip, uint32 port) {
		::memset(&_addr, 0, sizeof(_addr));
        _addr.sin6_family = AF_INET6;
        _addr.sin6_port = net::hton16(static_cast<uint16>(port));

        if (!ip.empty() && ip != "*") {
            int ret = ::inet_pton(AF_INET6, ip.c_str(), &_addr.sin6_addr);
			CHECK_EQ(ret, 1) << "invalid ip: " << ip;
        }
    }

	virtual ~ipv6_addr() {}

    virtual const struct sockaddr* addr() const {
        return (struct sockaddr*) &_addr;
    }

    virtual struct sockaddr* addr() {
        return (struct sockaddr*) &_addr;
    }

    virtual uint32 size() const {
        return sizeof(_addr);
    }

    virtual std::string to_string() const {
        char s[INET6_ADDRSTRLEN] = { 0 };
        inet_ntop(AF_INET6, &_addr.sin6_addr, s, sizeof(s));

        return std::string(s) + ":" +
               str::to_string(net::ntoh16(_addr.sin6_port));
    }
  private:
    struct sockaddr_in6 _addr;

    DISALLOW_COPY_AND_ASSIGN(ipv6_addr);
};

class unix_addr : public sock_addr {
  public:
    unix_addr() {
        ::memset(&_addr, 0, sizeof(_addr));
    }

    unix_addr(const std::string& path) {
		::memset(&_addr, 0, sizeof(_addr));

        CHECK_LT(path.size(), sizeof(_addr.sun_path)) << "too long: " << path;

        _addr.sun_family = AF_UNIX;
        _size = sizeof(_addr) - sizeof(_addr.sun_path) + path.size();

        ::memcpy(_addr.sun_path, path.data(), path.size());
    }

	~unix_addr() {}

    virtual const struct sockaddr* addr() const {
        return (struct sockaddr*) &_addr;
    }

    virtual struct sockaddr* addr() {
        return (struct sockaddr*) &_addr;
    }

    virtual uint32 size() const {
        return _size;
    }

    virtual std::string to_string() const {
        return std::string(_addr.sun_path);
    }

  private:
    struct sockaddr_un _addr;
    uint32 _size;

    DISALLOW_COPY_AND_ASSIGN(unix_addr);
};

//inline int epoll_create(int n = 10) {
//	return  ::epoll_create(n);
//}

//#define EV_READ 0
//#define EV_WRITE 1
//
//inline bool epoll_add_read_event(int epfd,int fd,struct epoll_event &ev) {
//	ev.events = EPOLLIN;
//	ev.data.fd = fd;
//	if (::epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
//		return false;
//	}else {
//		return true;
//	}
//}

inline int tcp_socket() {
	return ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//return ::socket(AF_INET, SOCK_STREAM, 0);
}

inline int udp_socket() {
    return ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

inline int unix_socket() {
	//return ::socket(AF_UNIX, SOCK_STREAM, IPPROTO_TCP);
	return ::socket(AF_UNIX, SOCK_STREAM, 0);
}

bool connect(int sockfd, const sock_addr& server);

// bind sockfd to a specific ip or port
bool bind(int sockfd, const sock_addr& addr);

bool listen(int sockfd, int backlog = 128);

bool setnonblocking(int sockfd);

int accept(int sockfd, sock_addr* cli);
int unixaccept(int sockfd, unix_addr &);

int32 read(int fd, void* ptr, uint32 n);
ssize_t readn(int fd, void *vptr, size_t n);

int32 write(int fd, const void* ptr, uint32 n);
ssize_t writen(int fd,const void *vptr,size_t n);

bool getsockname(int fd, sock_addr* addr);
bool getpeername(int fd, sock_addr* addr);

std::vector<std::string> get_ip_by_name(const std::string& name);

} // namespace net

/* todo: sock
namespace sock {

class socket {
  public:
    socket();
    ~socket();

    bool bind(const std::string& host, uint32 port);
    bool listen(int backlog = 128);
    std::shared_ptr<socket> accept();

    bool connect(const std::string& host, uint32 port);

    std::string recv(uint32 size);
    void send(const std::string& data);

    void sendall();

    // udp
    // return data and remote addr
    std::string recvfrom(uint32 size);
    void sendto(const std::string& msg, const std::string& host, uint32 port);

    void getpeername();
    void getsockname();
    void getsockopt();
    void setsockopt();
    void setblocking();
    void settimeout();
    void gettimeout();

    int fileno();

    void close();

};

socket* create_socket(uint32 family, uint32 type);
}
*/
