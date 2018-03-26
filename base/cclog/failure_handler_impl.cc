/*
 * see more on https://github.com/vmarkovtsev/DeathHandler
 */

#include "failure_handler.h"
#include "../sys.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <sys/wait.h>

#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/function.hpp>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>

#define INLINE __attribute__((always_inline)) inline

namespace cclog {
namespace xx {

typedef uint64_t uint64;
typedef uint32_t uint32;

typedef void* (*malloc_hook_t)(size_t size, const void*);
typedef void (*free_hook_t)(void*, const void*);

class FailureHandlerImpl : public FailureHandler {
  public:
    FailureHandlerImpl();
    virtual ~FailureHandlerImpl();

    virtual void set_fd(FILE* f) {
        _f = f;
    }

    virtual void set_handler(boost::function<void()> cb) {
        _cb = cb;
    }

  private:
    static char* _buf;
    static const int _buf_size;  // 12k
    static const int _nframes;
    static FILE* _f;
    static boost::function<void()> _cb;

    static void on_signal(int sig);

    static void* malloc_hook(size_t size, const void*) {
        return _buf + _buf_size - _nframes * sizeof(void*);
    }
};

char* FailureHandlerImpl::_buf = NULL;
const int FailureHandlerImpl::_buf_size = 12 * 1024;
const int FailureHandlerImpl::_nframes = 128;
FILE* FailureHandlerImpl::_f = NULL;
boost::function<void()> FailureHandlerImpl::_cb = NULL;

FailureHandler* NewFailureHandler() {
    return new FailureHandlerImpl;
}

//SA_RESTART：使被信号中断的系统调用能够重新发起
//SA_ONSTACK：Use the alternate signal stack if available so we can catch stack overflows.
FailureHandlerImpl::FailureHandlerImpl() {
    _buf = new char[_buf_size];

    int flag = SA_RESTART | SA_ONSTACK;
    sys::signal.add_handler(SIGSEGV, &FailureHandlerImpl::on_signal, flag);
    sys::signal.add_handler(SIGABRT, &FailureHandlerImpl::on_signal, flag);
    sys::signal.add_handler(SIGFPE, &FailureHandlerImpl::on_signal, flag);
}

FailureHandlerImpl::~FailureHandlerImpl() {
    sys::signal.del_handler(SIGSEGV);
    sys::signal.del_handler(SIGABRT);
    sys::signal.del_handler(SIGFPE);

    ::memset(_buf, 0, _buf_size);
    delete[] _buf;
}

INLINE char* u64_to_str(uint64 v, char* buf, int base = 10) {
    if (v == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    const int size = 32;
    buf[size - 1] = '\0';

    int i = size - 2;
    for (; v != 0; --i, v /= base) {
        buf[i] = "0123456789ABCDEF"[v % base];
    }

    return buf + i + 1;
}

INLINE char* ptr_to_str(const void* addr, char* buf) {
    char* cur = u64_to_str((uint64) addr, buf + 2, 16);
    *(cur - 2) = '0';
    *(cur - 1) = 'x';

    return cur - 2;
}

INLINE void write_msg(const char* msg, uint32 len = 0, FILE* f = NULL) {
    if (len == 0) len = strlen(msg);
    fwrite(msg, 1, len, stderr);
    if (f != NULL) fwrite(msg, 1, len, f);
}

INLINE void safe_abort() {
    ::kill(getppid(), SIGCONT);
    sys::signal.reset(SIGABRT);
    ::abort();
}

#define ABORT_IF(cond, msg) \
    if (cond) { \
        write_msg(msg); \
        safe_abort(); \
    }

static int addr2line(const char* image, const char* addr, char* buf) {
    int pipefd[2];
    ABORT_IF(pipe(pipefd) != 0, "create pipe failed");

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);

        int ret = execlp("addr2line", "addr2line", addr, "-f", "-C", "-e",
                         image, (void*) NULL);
        ABORT_IF(ret == -1, "execlp addr2line failed");
    }

    ::close(pipefd[1]);
    ABORT_IF(waitpid(pid, NULL, 0) != pid, "waitpid failed");

    ssize_t len = ::read(pipefd[0], buf, 4096);
    ::close(pipefd[0]);

    ABORT_IF(len == 0, "read zero bytes from pipe");
    buf[len] = '\0';

    return len;
}

#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

void FailureHandlerImpl::on_signal(int sig) {
    if (_cb != NULL) _cb();//执行回调
    if (_f != NULL) ::fflush(_f);//将缓冲区数据写入文件

    pid_t pid = fork();

    if (pid != 0) { /* parent process */
        int status;
        ::kill(getpid(), SIGSTOP);         // stop this process
        ::waitpid(pid, &status, WNOHANG);

        sys::signal.reset(SIGABRT);//重置异常x信号
        ::abort();
    }

    // in child process
    ::dup2(STDERR_FILENO, STDOUT_FILENO);

    // print signal info
    char* sig_info = _buf;
    sig_info[0] = '\0';

    switch (sig) {
      case SIGSEGV:
        strcat(sig_info, "<segmentation fault>");
        break;
      case SIGABRT:
        strcat(sig_info, "<aborted>");
        break;
      case SIGFPE:
        strcat(sig_info, "<floating point exception>");
        break;
      default:
        strcat(sig_info, "<caught unexpected signal>");
        break;
    }

    FILE* file = _f;
    write_msg(sig_info, strlen(sig_info), file);

    // backtrace
    void** addrs = (void**) _buf;

    // hook malloc and free
    malloc_hook_t old_malloc_hook = __malloc_hook;
    free_hook_t old_free_hook = __free_hook;

    __malloc_hook = &FailureHandlerImpl::malloc_hook;
    __free_hook = NULL;

    int nframes = ::backtrace(addrs, _nframes);

    // restore malloc and free hook
    __malloc_hook = old_malloc_hook;
    __free_hook = old_free_hook;

    ABORT_IF(nframes <= 2, "frames num <= 2");

    // program path
    char* image = _buf + nframes * sizeof(void*);
    ssize_t image_len = readlink("/proc/self/exe", image, 2048);
    ABORT_IF(image_len < 1, "readlink /proc/self/exe] image_len < 1");
    image[image_len] = '\0';

    // current work dir
    char* cwd = image + image_len + 1;
    ABORT_IF(getcwd(cwd, 2048) == NULL, "getcwd failed");
    strcat(cwd, "/");

    // addr2line: turn addrs to function names, numbers
    char* buf = cwd + strlen(cwd) + 1;

    for (int i = 2; i < nframes; ++i) { /* skip the first 2 frames */
        char* addr = buf;
        int addr_len;

        char* line;
        int line_len;

        Dl_info di;
        if (dladdr(addrs[i], &di) == 0 || di.dli_fname[0] != '/'
            || strcmp(image, di.dli_fname) == 0) {
            addr = ptr_to_str(addrs[i], addr);
            line = addr + (addr_len = strlen(addr)) + 1;
            line_len = addr2line(image, addr, line);

        } else {
            char* p = (char*) ((char*) addrs[i] - (char*) di.dli_fbase);
            addr = ptr_to_str(p, addr);
            line = addr + (addr_len = strlen(addr)) + 1;
            line_len = addr2line(di.dli_fname, addr, line);
        }

        char* line_prefix = line + line_len + 1;
        line_prefix = u64_to_str((uint64_t) (i - 2), line_prefix, 10);

        strcat(line_prefix, "  ");
        strcat(line_prefix, addr);
        strcat(line_prefix, " in ");

        write_msg("\n#", 2, file);
        write_msg(line_prefix, strlen(line_prefix), file);

        char* fend = strstr(line, "\n");
        ABORT_IF(fend == NULL, "find no '\n' in line");

        *fend = '\0';
        write_msg(line, strlen(line), file);  // function name
        write_msg(" at ", 4, file);

        line = fend + 1;
        if (*line == '?') {
            write_msg(image, image_len, file);
        } else {
            int len = strlen(line);
            if (len > 0) {
                line[len - 1] = '\0';  // overwrite '\n'
                write_msg(line, len - 1, file);
            }
        }
    }

    write_msg("\n", 1, file);
    if (file != NULL) fclose(file);

    ::kill(getppid(), SIGCONT);
    _Exit(EXIT_SUCCESS);
}

#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
#pragma GCC diagnostic pop
#endif

#undef INLINE
#undef ABORT_IF

}  // namespace xx
}  // namespace cclog
