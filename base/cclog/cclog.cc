#include "cclog.h"
#include "failure_handler.h"

#include "../os.h"
#include "../sys.h"
#include "../string_util.h"
#include "../thread_util.h"

#include <map>
#include <vector>
#include <memory>

#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/function.hpp>

DEF_bool(log2stderr, false, "log to stderr only");
DEF_bool(alsolog2stderr, false, "log to stderr and file");
DEF_bool(dlog_on, false, "if true, turn on DLOG");

DEF_string(log_dir, "log", "log dir");
DEF_string(log_prefix, "", "prefix of log file name");
DEF_int64(max_log_file_size, 1 << 30, "max log file size, default: 1G");

namespace cclog {
namespace xx {

static std::string kProgName = os::get_process_name();

//日志器的虚基类
class Logger {
  public:
    explicit Logger(uint32 ms);
	virtual ~Logger() {}

    void stop();

    static void set_log_dir(const std::string& log_dir) {
        _log_dir = log_dir;
        if (!_log_dir.empty() && *_log_dir.rbegin() != os::path.sep) {
            _log_dir += os::path.sep;
        }
    }

    static void set_log_prefix(const std::string& log_prefix) {
        _log_prefix = log_prefix;
        if (!_log_prefix.empty() && *_log_prefix.rbegin() != '.') {
            _log_prefix.push_back('.');
        }
    }

  protected:
    safe::Mutex _log_mtx;
	boost::scoped_ptr<safe::StoppableThread> _log_thread;
    uint32 _ms; // run thread_fun() every n ms

    std::vector<void*> _logs;
    std::vector<void*> _temp;

    uint32 _last_day;
    uint32 _last_hour;

    static std::string _log_dir;
    static std::string _log_prefix;

  private:
    void thread_fun();

    virtual void flush_log_files() = 0;
    virtual void write_logs(std::vector<void*>& logs) = 0;

    DISALLOW_COPY_AND_ASSIGN(Logger);
};

std::string Logger::_log_dir;
std::string Logger::_log_prefix;

Logger::Logger(uint32 ms)
    : _ms(ms) {
    _log_thread.reset(
        new safe::StoppableThread(boost::bind(&Logger::thread_fun, this), _ms));//创建一个定时运行且可停止的线程
    _log_thread->start();

    int64 sec = sys::local_time.sec();
    _last_day = sec / 86400;
    _last_hour = sec / 3600;
}

//定时运行该工作函数
void Logger::thread_fun() {
    {
        safe::MutexGuard g(_log_mtx);
        if (!_logs.empty()) _logs.swap(_temp);//将数据交换到另一块内存
    }

    this->write_logs(_temp);//将交换出的数据写入到各日志类型的存储器
    _temp.clear();

    if (_temp.capacity() > 1024) {//回收临时内存区域
        std::vector<void*>().swap(_temp);
        _temp.reserve(1024);
    }

    this->flush_log_files();//将各种类型日志写入到日志文件
}

void Logger::stop() {
    _log_thread->join(); // wait for logging thread

    safe::MutexGuard g(_log_mtx);
    if (_logs.empty()) return;//如果缓冲区里没有数据则直接退出

    this->write_logs(_logs);//如果日志缓冲区有数据，将数据写入到各类型日志器
    this->flush_log_files();//将各日志器数据写入文件
    _logs.clear();//释放内存
}

class TaggedLogger : public Logger {
  public:
    TaggedLogger() : Logger(500) {//doing work per 500ms
    }

    virtual ~TaggedLogger() {
        Logger::stop();
    }

    void push(TaggedLog* log) {
        safe::MutexGuard g(_log_mtx);
        _logs.push_back(log);
    }

    enum strategy {
        by_day = 0,
        by_hour = 1,
    };

    void log_by_day(const char* tag) {
        _strategy[tag] = by_day;
    }

    void log_by_hour(const char* tag) {
        _strategy[tag] = by_hour;
    }

  private:
    std::map<const char*, os::file> _files;
    std::map<const char*, int> _strategy;

    bool open_log_file(const char* tag);
    void log_to_file(const std::string& time, TaggedLog* log);

    virtual void flush_log_files();
    virtual void write_logs(std::vector<void*>& logs);
};

//打开日志文件
bool TaggedLogger::open_log_file(const char* tag) {
    std::string time =
        sys::local_time.to_string(_strategy[tag] == 0 ? "%Y%m%d" : "%Y%m%d%H");

    std::string name = _log_prefix + kProgName + "." + time + "." + tag;
    std::string path = _log_dir + name;

    if (!os::path.exists(_log_dir)) os::makedirs(_log_dir);
    if (!_files[tag].open(path, "a")) return false;

    std::string link_path = _log_dir + kProgName + "." + tag;
    os::symlink(name, link_path);

    return true;
}

//将各类型日志数据写入到对应文件
inline void TaggedLogger::log_to_file(const std::string& time, TaggedLog* log) {
	os::file& file = _files[log->type()];//获取该日志文件
    if (!file && !this->open_log_file(log->type())) return;

    file.write(time);
    file.write(log->data(), log->size());
}

//实现虚函数(将各类型日志数据写入到对应文件)
void TaggedLogger::write_logs(std::vector<void*>& logs) {
    std::string time(sys::local_time.to_string()); // yyyy-mm-dd hh:mm:ss

    for (::size_t i = 0; i < logs.size(); ++i) {
        TaggedLog* log = (TaggedLog*) logs[i];
        this->log_to_file(time, log);
        delete log;
    }
}

//将缓冲区数据写入到文件
void TaggedLogger::flush_log_files() {
    uint64 ms = sys::local_time.ms() + _ms;
    uint32 day = ms / (86400 * 1000);
    uint32 hour = ms / (3600 * 1000);

    if (day != _last_day) std::swap(day, _last_day);
    if (hour != _last_hour) std::swap(hour, _last_hour);

    for (std::map<const char*, os::file>::iterator 
		it = _files.begin(); it != _files.end(); ++it) {
		os::file& file = it->second;

        if (file.exists()) file.flush();
        if (file.exists() && day == _last_day &&
            (hour == _last_hour || _strategy[it->first] != by_hour)) {
            continue;
        }

        file.close();
    }
}

enum LogLevel {
    INFO = 0,
    WARNING = 1,
    ERROR = 2,
    FATAL = 3,
};

class LevelLogger : public Logger {
  public:
    LevelLogger();
    virtual ~LevelLogger();

    void push_non_fatal_log(LevelLog* log) {
        safe::MutexGuard g(_log_mtx);
        _logs.push_back(log);
    }

    void push_fatal_log(LevelLog* log);

    void install_failure_handler();

    void uninstall_failure_handler() {
        _failure_handler.reset();
    }

    void install_signal_handler();
    void uninstall_signal_handler();

  private:
    std::vector<os::file> _files;
    std::vector<int> _index;
    boost::scoped_ptr<FailureHandler> _failure_handler;

    bool open_log_file(int level, const char* tag);
    void log_to_file(const std::string& time, LevelLog* log);
    void log_to_stderr(const std::string& time, LevelLog* log);

    void on_failure();        // for CHECK failed, SIGSEGV, SIGFPE
    void on_signal(int sig);  // for SIGTERM, SIGINT, SIGQUIT

    virtual void flush_log_files();
    virtual void write_logs(std::vector<void*>& logs);
};

LevelLogger::LevelLogger() : Logger(1000) {
    _files.resize(FATAL + 1);
    _index.resize(FATAL + 1);
    this->install_signal_handler();
}

LevelLogger::~LevelLogger() {
    Logger::stop();
    this->uninstall_signal_handler();
}

void LevelLogger::install_failure_handler() {
    if (_failure_handler != NULL) return;
    _failure_handler.reset(NewFailureHandler());
    _failure_handler->set_handler(boost::bind(&LevelLogger::on_failure, this));
}

void LevelLogger::install_signal_handler() {
    //auto f = boost::bind(&LevelLogger::on_signal, this, boost::placeholders::_1);
	//using namespace boost;
	//boost::function<void(int)> f = boost::bind(&LevelLogger::on_signal, this, _1);

    //sys::signal.add_handler(SIGTERM, boost::bind(f, SIGTERM));
	sys::signal.add_handler(SIGTERM, boost::bind(&LevelLogger::on_signal, this, SIGTERM), SA_RESTART | SA_ONSTACK);
    sys::signal.add_handler(SIGQUIT, boost::bind(&LevelLogger::on_signal, this, SIGQUIT), SA_RESTART | SA_ONSTACK);
    sys::signal.add_handler(SIGINT, boost::bind(&LevelLogger::on_signal, this, SIGINT), SA_RESTART | SA_ONSTACK);
    sys::signal.ignore(SIGPIPE);
}

void LevelLogger::uninstall_signal_handler() {
    sys::signal.del_handler(SIGTERM);
    sys::signal.del_handler(SIGQUIT);
    sys::signal.del_handler(SIGINT);
}

void LevelLogger::on_failure() {
    ::cclog::close_cclog();

    if (!_files[FATAL]) this->open_log_file(FATAL, "FATAL");
    _failure_handler->set_fd(_files[FATAL].fd());
}

void LevelLogger::on_signal(int sig) {
    ::cclog::close_cclog();
    if (sig != SIGTERM) return;

    std::string msg("terminated: probably killed by someone!\n");
    ::fwrite(msg.data(), 1, msg.size(), stderr);

    if (_files[FATAL] || this->open_log_file(FATAL, "FATAL")) {
        _files[FATAL].write(msg);
        _files[FATAL].flush();
    }
}

bool LevelLogger::open_log_file(int level, const char* tag) {
	os::file& file = _files[level];

    std::string time = sys::local_time.to_string("%Y%m%d");  // yyyymmdd
    std::string name = _log_prefix + kProgName + "." + time; // + "." + tag;

    if (file.exists()) {
        name += std::string(1, '_') + str::to_string(++_index[level]);
    }

    name += std::string(1, '.') + tag;
    std::string path = _log_dir + name;

    if (!os::path.exists(_log_dir)) os::makedirs(_log_dir);
    if (!file.open(path, "a")) return false;

    std::string link_path = _log_dir + kProgName + "." + tag;
    os::symlink(name, link_path);

    return true;
}

inline void LevelLogger::log_to_stderr(const std::string& time, LevelLog* log) {
    ::fwrite("IWEF" + log->type(), 1, 1, stderr);
    ::fwrite(time.data(), 1, time.size(), stderr);
    ::fwrite(log->data(), 1, log->size(), stderr);
}

void LevelLogger::write_logs(std::vector<void*>& logs) {
    std::string time(sys::local_time.to_string("%m%d %H:%M:%S"));

    if (!FLG_log2stderr && !FLG_alsolog2stderr) { /* default: log to file */
        for (::size_t i = 0; i < logs.size(); ++i) {
            LevelLog* log = (LevelLog*) logs[i];
            this->log_to_file(time, log);
            delete log;
        }

    } else if (FLG_alsolog2stderr) { /* log to stderr and file */
        for (::size_t i = 0; i < logs.size(); ++i) {
            LevelLog* log = (LevelLog*) logs[i];
            this->log_to_stderr(time, log);
            this->log_to_file(time, log);
            delete log;
        }

    } else { /* log to stderr */
        for (::size_t i = 0; i < logs.size(); ++i) {
            LevelLog* log = (LevelLog*) logs[i];
            this->log_to_stderr(time, log);
            delete log;
        }
    }
}

#define WRITE_LOGS(time, log, i) \
    do { \
        os::file& file = _files[i]; \
        if (!file && !this->open_log_file(i, #i)) return; \
        file.write("IWEF"[log->type()]); \
        file.write(time); \
        file.write(log->data(), log->size()); \
    } while (0)

void LevelLogger::log_to_file(const std::string& time, LevelLog* log) {
    WRITE_LOGS(time, log, INFO);
    if (log->type() < WARNING) return;

    WRITE_LOGS(time, log, WARNING);
    if (log->type() < ERROR) return;

    WRITE_LOGS(time, log, ERROR);
}

void LevelLogger::push_fatal_log(LevelLog* log) {
    ::cclog::close_cclog();

    std::string time(sys::local_time.to_string("%m%d %H:%M:%S"));
    WRITE_LOGS(time, log, FATAL);
    _files[FATAL].flush();

    this->log_to_stderr(time, log);
    delete log;

    if (_failure_handler == NULL) exit(0);

    _failure_handler->set_handler(NULL);
    _failure_handler->set_fd(_files[FATAL].fd());
    ::abort();
}

#undef WRITE_LOGS

void LevelLogger::flush_log_files() {
    uint32 day = sys::local_time.sec() / 86400;

    // reset index on new day
    if (day != _last_day) {
        std::swap(day, _last_day);
        _index.clear();
        _index.resize(FATAL + 1);
    }

    for (uint32 i = 0; i < _files.size(); ++i) {
		os::file& file = _files[i];

        if (file.exists()) file.flush();
        if (file.exists() && file.size() < ::FLG_max_log_file_size) continue;

        file.close();
    }
}

static TaggedLogger kTaggedLogger;
static LevelLogger kLevelLogger;//静态日志对象

TaggedLogSaver::~TaggedLogSaver() {
    (*_log) << '\n';
    kTaggedLogger.push(_log);
}

void NonFatalLogSaver::push() {
    kLevelLogger.push_non_fatal_log(_log);
}

void FatalLogSaver::push() {
    kLevelLogger.push_fatal_log(_log);
}

}  // namespace xx

void init_cclog(const std::string& argv0) {
	std::pair<std::string, std::string> it = os::path.split(argv0);
	//if (!it.second.empty()) xx::kProgName = std::move(it.second);
	if (!it.second.empty()) xx::kProgName = it.second;

    xx::Logger::set_log_dir(::FLG_log_dir);
    xx::Logger::set_log_prefix(::FLG_log_prefix);
    xx::kLevelLogger.install_failure_handler();
}

void close_cclog() {
    xx::kTaggedLogger.stop();
    xx::kLevelLogger.stop();
}

void log_by_day(const char* tag) {
    xx::kTaggedLogger.log_by_day(tag);
}

void log_by_hour(const char* tag) {
    xx::kTaggedLogger.log_by_hour(tag);
}

}  // namespace cclog
