#pragma once

#include "data_types.h"
#include "os.h"

#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>

#include <string>
//#include <functional>//since c++11
#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace sys {
namespace xx {

/*
 * univeral time(UTC)
 *
 *   sec():  seconds since EPOCH
 *    ms():  milliseconds since EPOCH
 *    us():  microseconds since EPOCH
 */
struct utc {
    static inline int64 us() {
        struct timeval now;
        ::gettimeofday(&now, NULL);
        return static_cast<int64>(now.tv_sec) * 1000000 + now.tv_usec;
    }

    static inline int64 ms() {
        return utc::us() / 1000;
    }

    static inline int64 sec() {
        return utc::us() / 1000000;
    }

    static inline int64 hour() {
        return utc::sec() / 3600;
    }

    static inline int64 day() {
        return utc::sec() / 86400;
    }

    static inline bool is_today(int64 sec) {
        return utc::day() == sec / 86400;
    }
};

/*
 * local time
 */
struct local_time {
    static inline int64 us() {
        struct timeval now;
        struct timezone tz;
        ::gettimeofday(&now, &tz);

        return now.tv_usec +
            (static_cast<int64>(now.tv_sec) - tz.tz_minuteswest * 60) * 1000000;
    }

    static inline int64 ms() {
        return local_time::us() / 1000;
    }

    static inline int64 sec() {
        return local_time::us() / 1000000;
    }

    static inline int64 hour() {
        return local_time::sec() / 3600;
    }

    static inline int64 day() {
        return local_time::sec() / 86400;
    }

    static inline bool is_today(int64 sec) {
        return local_time::day() == sec / 86400;
    }

    static std::string to_string(int64 sec,
                                 const char* format = "%Y-%m-%d %H:%M:%S");

    static inline std::string to_string() {
        return local_time::to_string(local_time::sec());
    }

    /*
     * to_string("%Y-%m-%d %H:%M:%S") ==> 2015-12-25 01:23:45
     */
    static inline std::string to_string(const char* format) {
        return local_time::to_string(local_time::sec(), format);
    }
};

/*
 * write to or read from system clipboard, **xclip** required.
 */
struct clipboard {
    static void write(const std::string& s) {
        os::system("echo -n \"" + s + "\" |xclip -i -selection clipboard");
    }

    static std::string read() {
        return os::system("xclip -o -selection clipboard");
    }
};

/*
 * signal_handler:
 *
 *   for setting multiple handlers to one signal
 *
 *   os::signal.add_handler(SIGINT, std::function<void()> a);
 *   os::signal.add_handler(SIGINT, std::function<void()> b);
 *
 * SIGSEGV, 11, segmentation fault
 * SIGABRT, 6,  abort()
 * SIGFPE,  8,  devided by zero
 * SIGTERM, 15, terminate, default signal by kill
 * SIGINT,  2,  Ctrl + C
 * SIGQUIT, 3,  Ctrl + \
 */

typedef void (*handler_t)(int);

struct signal {
    static void add_handler(int sig, boost::function<void()> cb, int flag = 0);

    static void add_handler(int sig, handler_t handler, int flag = 0) {
        signal::add_handler(sig, boost::bind(handler, sig), flag);
    }

    static void del_handler(int sig);

    // set handler to SIG_IGN
    static inline bool ignore(int sig) {
        return set_handler(sig, SIG_IGN);
    }

    // set handler to SIG_DFL
    static inline void reset(int sig) {
        set_handler(sig, SIG_DFL);
    }

    static inline void kill(int sig) {
        ::kill(getpid(), sig);
    }

    static inline void kill(int pid, int sig) {
        ::kill(pid, sig);
    }

    // set handler for signal @sig
    static bool set_handler(int sig, handler_t handler, int flag = 0);

    // get old handler of signal @sig
    static handler_t get_handler(int sig);
};

//单例：信号处理句柄
class signal_handler {
  public:
    static signal_handler* instance() {
        static signal_handler sh;//局部静态变量值只创建一次
        return &sh;
    }

    void add_handler(int sig, boost::function<void()> cb, int flag = 0);
    void del_handler(int sig);

  private:
	  signal_handler() {}
	  ~signal_handler() {}

    static void on_signal(int sig) {
        signal_handler::instance()->handle_signal(sig);
    }

    typedef std::deque<boost::function<void()> > cbque;
    void handle_signal(int sig);

  private:
    std::map<int, cbque> _map;//每种信号对应的多种信号回调映射

    DISALLOW_COPY_AND_ASSIGN(signal_handler);//since c++11
};

//注册信号回调（将回调注册到信号处理句柄）
inline void signal::add_handler(int sig, boost::function<void()> cb, int flag) {
	signal_handler::instance()->add_handler(sig, cb, flag);//注册信号对应回调
}

//删除信号回调（将回调从信号处理句柄中删除）
inline void signal::del_handler(int sig) {
	signal_handler::instance()->del_handler(sig);
}

} // namespace xx

extern xx::utc utc;
extern xx::local_time local_time;
extern xx::clipboard clipboard;
extern xx::signal signal;


/***** time utility *****/
/*
 * sleep for @ms milliseconds
 */
void msleep(uint32 ms);

/*
 * sleep for @us microseconds
 */
void usleep(uint32 us);

/*
 * sleep for @sec seconds
 */
inline void sleep(uint32 sec) {
    sys::msleep(sec * 1000);
}

/*
 * usage:
 *     os::timer t;
 *     os::msleep(100);
 *     CERR << "time elapse: " << t.ms();
 */
class timer {
  public:
    timer() {
        _start = sys::utc.us();
    }
	~timer() {}

    void restart() {
        _start = sys::utc.us();
    }

    int64 us() const {
        return sys::utc.us() - _start;
    }

    int64 ms() const {
        return this->us() / 1000;
    }

    int64 sec() const {
        return this->us() / 1000000;
    }

  private:
    int64 _start;

    DISALLOW_COPY_AND_ASSIGN(timer);//since c++11
};

} // namespace sys
