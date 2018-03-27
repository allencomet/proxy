#include "sys.h"
#include "cclog/cclog.h"
#include "errno.h"

namespace sys {
namespace xx {

//将本地时间转换成字符串
std::string local_time::to_string(int64 sec, const char* format) {
    struct tm tm;
    ::gmtime_r(&sec, &tm);

    std::string s(32, '\0');
    size_t r = ::strftime(&s[0], 32, format, &tm);
    s.resize(r);

    return s;
}

//注册信号回调
bool signal::set_handler(int sig, handler_t handler, int flag) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    sa.sa_flags = flag;
    sa.sa_handler = handler;

    return sigaction(sig, &sa, NULL) != -1;
}

//获取信号回调
handler_t signal::get_handler(int sig) {
    struct sigaction sa;
    sigaction(sig, NULL, &sa);
    return sa.sa_handler;
}

//注册信号回调(自主维护一个回调列表,在信号触发后逐一调用)
void signal_handler::add_handler(int sig, boost::function<void()> cb, int flag) {
    std::map<int, cbque>::iterator it = _map.find(sig);
    cbque& cbs = (it != _map.end()) ? it->second : (_map[sig] = cbque());//不存在则新建一个
    if (it != _map.end()) {
        cbs.push_back(cb);
        return;
    }

    handler_t oh = signal::get_handler(sig);
    if (oh != SIG_DFL && oh != SIG_IGN && oh != SIG_ERR) {
        cbs.push_back(boost::bind(oh, sig));//添加原信号处理回调
    }

    signal::set_handler(sig, &signal_handler::on_signal, flag);
    cbs.push_back(cb);//追加最新注册的信号回调函数
}

//删除信号对应所有回调
void signal_handler::del_handler(int sig) {
    _map.erase(sig);//从信号处理列表中删除
    signal::reset(sig);//重置该信号为默认处理
}

//信号回调：处理信号(只处理一次，触发一次后所有回调逐一运行后删除)
void signal_handler::handle_signal(int sig) {
    std::map<int, cbque>::iterator it = _map.find(sig);
    if (it == _map.end()) return;

    cbque& cbs = it->second;

    while (!cbs.empty()) {
        boost::function<void()> cb = cbs.back();
        cbs.pop_back();
        cb();
    }
}

} // namespace xx

//睡眠按毫秒计算
void msleep(uint32 ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = ms % 1000 * 1000000;

    while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
}

//睡眠按微秒计算
void usleep(uint32 us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = us % 1000000 * 1000;

    while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
}

} // namespace sys
