#pragma once

#include "../data_types.h"
#include "../stream_buf.h"
#include "../ccflag/ccflag.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>            // for syscall
#include <sys/syscall.h>       // for SYS_gettid

DEC_bool(stop_server);         // stop running server
DEC_bool(log2stderr);          // log to stderr only
DEC_bool(alsolog2stderr);      // log to stderr and file
DEC_bool(dlog_on);             // if true, turn on DLOG
DEC_string(log_dir);           // log dir, created if not exists
DEC_string(log_prefix);        // prefix of log file name
DEC_int64(max_log_file_size);  // max log file size for LevelLog



namespace cclog {
namespace std = ::std;

/*
 * @argv0  argv[0] or any valid string for a file name.
 */
void init_cclog(const std::string& argv0);

/*
 * write all buffered logs to destination and stop the logging thread.
 */
void close_cclog();

/*
 * log_by_day("xx") ==> one log file per day for TLOG("xx")
 */
void log_by_day(const char* tag);

/*
 * log_by_hour("xx") ==> one log file per hour for TLOG("xx")
 */
void log_by_hour(const char* tag);

/*
* tagged-log
*
*   Tagged-logs are buffered for performance.
*
*   TLOG("tag") ==> log to file xx_tag_xx.log
*   DLOG("tag") ==> log to file xx_dlog_tag_xx.log only if FLG_dlog_on == true
*
*   TLOG("foobar") << "hello world" << 23;
*/
#define TLOG(T) ::cclog::xx::TaggedLogSaver(__FILE__, __LINE__, T).sb()
#define TLOG_IF(T, cond) if (cond) TLOG(T)
#define DLOG(T) TLOG_IF("dlog_" T, ::FLG_dlog_on)

/*
* log to stderr with file name and line number.
*
*   CERR << "hello world" << 123;
*/
#define CERR ::cclog::xx::CerrSaver().sb()
#define COUT ::cclog::xx::CoutSaver().sb()

/*
* level-log
*
*   All level of logs are buffered for performance.
*
*    LOG ==> INFO      log to file: xx.INFO
*   WLOG ==> WARNING   log to file: xx.{ WARNING, INFO }
*   ELOG ==> ERROR     log to file: xx.{ ERROR, WARNING, INFO }
*   FLOG ==> FATAL     log to file: xx.FATAL and terminate the process.
*
*   CHECK ==> FATAL if check failed
*
*   LOG << "hello world" << true;
*   CHECK(1 + 1 == 3) << "1 + 1 != 3";   // check failed!
*   CHECK_EQ(1 + 1, 2) << "1 + 1 != 2";  // ok
*/
#define LOG ::cclog::xx::NonFatalLogSaver(__FILE__, __LINE__, 0).sb()
#define WLOG ::cclog::xx::NonFatalLogSaver(__FILE__, __LINE__, 1).sb()
#define ELOG ::cclog::xx::NonFatalLogSaver(__FILE__, __LINE__, 2).sb()
#define FLOG ::cclog::xx::FatalLogSaver(__FILE__, __LINE__, 3).sb() \
                 << "fatal error! "

/*
* XLOG_IF(cond): log only if cond == true
*/
#define LOG_IF(cond) if (cond) LOG
#define WLOG_IF(cond) if (cond) WLOG
#define ELOG_IF(cond) if (cond) ELOG
#define FLOG_IF(cond) if (cond) FLOG

#define CHECK(cond) \
    if (!(cond)) \
        ::cclog::xx::FatalLogSaver(__FILE__, __LINE__, 3).sb() \
            << "check failed: " #cond "! "

#define CHECK_NOTNULL(ptr) \
    if (ptr == NULL) \
        ::cclog::xx::FatalLogSaver(__FILE__, __LINE__, 3).sb() \
            << "check failed: " #ptr " mustn't be NULL! "

//#define CHECK_OP(a, b, op) \
   							   //    for (auto _x_ = std::make_pair(a, b); !(_x_.first op _x_.second);) \
//        ::cclog::xx::FatalLogSaver(__FILE__, __LINE__, 3).sb() \
//            << "check failed: " #a " " #op " " #b ", " \
//            << _x_.first << " vs " << _x_.second

#define CHECK_OP(a, b, op) \
    if(!(a op b)) \
        ::cclog::xx::FatalLogSaver(__FILE__, __LINE__, 3).sb() \
            << "check failed: " #a " " #op " " #b ", " \
            << a << " vs " << b

#define CHECK_EQ(a, b) CHECK_OP(a, b, ==)
#define CHECK_NE(a, b) CHECK_OP(a, b, !=)
#define CHECK_GE(a, b) CHECK_OP(a, b, >=)
#define CHECK_LE(a, b) CHECK_OP(a, b, <=)
#define CHECK_GT(a, b) CHECK_OP(a, b, >)
#define CHECK_LT(a, b) CHECK_OP(a, b, <)

namespace xx {

//标签style日志缓冲区
class TaggedLog : public ::StreamBuf {
  public:
    TaggedLog(const char* type, int size)
        : ::StreamBuf(size), _type(type) {
    }

	~TaggedLog() {}

    const char* type() const {
        return _type;
    }

  private:
    const char* _type;
};

//级别style日志缓冲区
class LevelLog : public ::StreamBuf {
public:
	LevelLog(int type, int size)
		: ::StreamBuf(size), _type(type) {
	}

	~LevelLog() {}

    int type() const {
        return _type;
    }

  private:
    int _type;
};

//标签style日志存储器
class TaggedLogSaver {
  public:
    TaggedLogSaver(const char* file, int line, const char* tag) {
        _log = new TaggedLog(tag, 256);
		(*_log) << ' ' << file << ':' << line << "] ";
    }

    ~TaggedLogSaver();

    ::StreamBuf& sb() {
        return *_log;
    }

  private:
    TaggedLog* _log;

    DISALLOW_COPY_AND_ASSIGN(TaggedLogSaver);
};

//标准错误输出缓冲区
class CerrSaver {
public:
	CerrSaver() {}

    ~CerrSaver() {
        _sb << '\n';
        ::fwrite(_sb.data(), 1, _sb.size(), stderr);
    }

    ::StreamBuf& sb() {
        return _sb;
    }

  private:
    ::StreamBuf _sb;

    DISALLOW_COPY_AND_ASSIGN(CerrSaver);
};

//标准输出缓冲区
class CoutSaver {
  public:
	  CoutSaver() {}

    ~CoutSaver() {
        _sb << '\n';
        ::fwrite(_sb.data(), 1, _sb.size(), stdout);
        ::fflush(stdout);
    }

    ::StreamBuf& sb() {
        return _sb;
    }

  private:
    ::StreamBuf _sb;

    DISALLOW_COPY_AND_ASSIGN(CoutSaver);
};

//级别style日志存储器
class LevelLogSaver {
  public:
    LevelLogSaver(const char* file, int line, int type) {
        _log = new LevelLog(type, 128);
        (*_log) << ' ' << syscall(SYS_gettid) << ' ' << file << ':' << line
                << "] ";
    }

	~LevelLogSaver() {}

    ::StreamBuf& sb() {
        return *_log;
    }

  protected:
    LevelLog* _log;

    DISALLOW_COPY_AND_ASSIGN(LevelLogSaver);
};

//非致命级别日志存储器
struct NonFatalLogSaver : public LevelLogSaver {
    NonFatalLogSaver(const char* file, int line, int type)
        : LevelLogSaver(file, line, type) {
    }

    ~NonFatalLogSaver() {
        this->sb() << '\n';
        this->push();
    }

    void push();
};

//致命级别日志存储器
struct FatalLogSaver : public LevelLogSaver {
    FatalLogSaver(const char* file, int line, int type)
        : LevelLogSaver(file, line, type) {
    }

    ~FatalLogSaver() {
        this->sb() << '\n';
        this->push();
    }

    void push();
};

}  // namespace xx
}  // namespace cclog




