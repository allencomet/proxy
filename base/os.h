#pragma once

#include "data_types.h"
#include "string_util.h"

#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/stat.h>

#include <string>
#include <vector>
#include <map>
#include <deque>
//#include <functional>
#include <boost/function.hpp>

namespace os {

/*
 * usage:
 *     os::system("rm -rf *");
 *     auto x = os::system("ls -l");
 */
std::string system(const std::string& cmd);

namespace xx {

struct path {
    static const char sep = '/';

    static std::string basename(const std::string& path) {
        std::string::size_type pos = path.rfind(sep);
        return pos != path.npos ? path.substr(pos + 1) : path;
    }

    static std::string dirname(const std::string& path) {
        std::string::size_type pos = path.rfind(sep);
        if (pos == path.npos) return std::string();

        return pos == 0 ? path.substr(0, 1) : path.substr(0, pos);
    }

    static std::pair<std::string, std::string> split(const std::string& path) {
        std::string::size_type pos = path.rfind(sep);

        std::string dir;
        if (pos != path.npos) {
            dir = (pos == 0 ? path.substr(0, 1) : path.substr(0, pos));
        }

        std::string base = (pos != path.npos ? path.substr(pos + 1) : path);

        return std::make_pair(dir, base);
    }

    static int64 atime(const std::string& path) {
        struct stat st;
        return ::stat(path.c_str(), &st) == 0 ? st.st_atim.tv_sec : -1;
    }

    static int64 ctime(const std::string& path) {
        struct stat st;
        return ::stat(path.c_str(), &st) == 0 ? st.st_ctim.tv_sec : -1;
    }

    static int64 mtime(const std::string& path) {
        struct stat st;
        return ::stat(path.c_str(), &st) == 0 ? st.st_mtim.tv_sec : -1;
    }

    static bool exists(const std::string& path) {
        return ::access(path.c_str(), F_OK) == 0;
    }

    static bool isdir(const std::string& path) {
        struct stat st;
        return ::lstat(path.c_str(), &st) == 0 ? S_ISDIR(st.st_mode) : false;
    }

    static bool isfile(const std::string& path) {
        struct stat st;
        return ::lstat(path.c_str(), &st) == 0 ? S_ISREG(st.st_mode) : false;
    }

    static bool islink(const std::string& path) {
        struct stat st;
        return ::lstat(path.c_str(), &st) == 0 ? S_ISLNK(st.st_mode) : false;
    }

    static int64 size(const std::string& path) {
        struct stat st;
        return ::stat(path.c_str(), &st) == 0 ? st.st_size : -1;
    }
};

} // namespace xx

extern xx::path path;

class file {
  public:
    file()
        : _mode("r"), _fd(NULL), _mtime(-1), _lineno(0), _eof(false) {
    }

    explicit file(const std::string& path, const std::string& mode = "") {
		_mode = "r";
		_fd = NULL;
		_mtime = -1;
		_lineno = 0;
		_eof = false;
        this->open(path, mode);
    }

    ~file() {
        this->close();
    }

    bool exists() const {
        return os::path.exists(_path);
    }

    const std::string& path() const {
        return _path;
    }

    std::string name() {
        return os::path.basename(_path);
    }

    int64 size() const {
        return os::path.size(_path);
    }

    int32 fileno() {
        return _fd == NULL ? -1 : ::fileno(_fd);
    }

    ::FILE* fd() const {
        return _fd;
    }

    // end of file
    bool eof() const {
        return _eof;
    }

    operator bool() const {
        return _fd != NULL;
    }

    bool modified() {
        return os::path.mtime(_path) != _mtime;
    }

    bool open(const std::string& path = std::string(),
              const std::string& mode = "");

    void close();

    void flush() {
        if (_fd != NULL) ::fflush(_fd);
    }

    /*
     * read size bytes, if size == -1, read all data
     */
    std::string read(uint32 size = -1);

    uint32 write(const void* s, uint32 size) {
        return ::fwrite(s, 1, size, _fd);
    }

    uint32 write(const std::string& s) {
        return this->write(s.data(), s.size());
    }

    uint32 write(const char c) {
        return this->write(&c, 1);
    }

    template <typename T>
    file& operator<<(const T& t) {
        this->write(str::to_string(t));
        return *this;
    }

    bool seek(int64 off, int whence = 0 /* 0: begin, 1: current, 2: end */) {
        int ret = ::fseek(_fd, off, whence);
        if (ret == 0) _eof = (this->tell() >= this->size());
        return ret == 0;
    }

    std::string getline();

    std::vector<std::string> getlines();

    int32 lineno() {
        return _lineno;
    }

    /*
     * current pos
     */
    int64 tell(){
        return ::ftell(_fd);
    }

    bool truncate(int64 size = 0) {
        return ::ftruncate(this->fileno(), size) == 0;
    }

  private:
    std::string _mode;
    std::string _path;
    ::FILE* _fd;

    int64 _mtime;
    int _lineno;
    bool _eof;
};


/***** basic file system operation *****/

inline bool mkdir(const std::string& path) {
    return ::mkdir(path.c_str(), 0755) == 0;
}

/*
 * remove empty directory
 */
inline bool rmdir(const std::string& path) {
    return ::rmdir(path.c_str()) == 0;
}

/*
 * remove all level directory
 */
bool rmdirs(const std::string& path);

std::vector<std::string> readdir(const std::string& path);

/*
 * mkdir -p
 */
inline void makedirs(const std::string& path) {
    os::system("mkdir -p " + path);
}

/*
 * remove non-dir file
 */
inline bool unlink(const std::string& path) {
    if (os::path.isdir(path)) return false;
    return ::unlink(path.c_str()) == 0;
}

inline bool symlink(const std::string& target, const std::string& link) {
    os::unlink(link);
    return ::symlink(target.c_str(), link.c_str()) == 0;
}

inline bool rename(const std::string& old_name, const std::string& new_name) {
    return ::rename(old_name.c_str(), new_name.c_str()) == 0;
}

/*
* Get working directory
*/
inline std::string getcwd() {
    char buf[4096] = { 0 };
    ::getcwd(buf, 4096);
    return std::string(buf);
}

/*
* change working directory
*/
inline bool chdir(const std::string& path) {
	return 0 == ::chdir(path.c_str());
}

/*
* fchdir() is identical to chdir(); 
* the only difference is that the directory is given as an open file descriptor.
*/
inline bool fchdir(int fd) {
	return 0 == ::fchdir(fd);
}

/*
* Get working directory
*/
inline std::string get_current_dir_name() {
	char *path = ::getcwd(NULL, NULL);
	return NULL != path ? std::string(path) : std::string();
}

/*
 * get current process id
 */
inline int32 getpid() {
    return ::getpid();
}

/*
 * get parent process id
 */
inline int32 getppid() {
    return ::getppid();
}

inline std::string readlink(const std::string& path) {
    char buf[4096] = { 0 };
    int r = ::readlink(path.c_str(), buf, 4096);
    return r > 0 ? std::string(buf, r) : std::string();
}

/*
 * get current process name
 */
inline std::string get_process_name() {
    return os::path.basename(os::readlink("/proc/self/exe"));
}

/*
 * get home directory of current user
 */
inline std::string homedir() {
    char* home = getenv("HOME");
    if (home != NULL) return std::string(home);

    struct passwd* p = getpwuid(getuid());
    if (p != NULL) return std::string(p->pw_dir);

    return std::string();
}

/*
 * run in the background
 */
inline bool daemon() {
    return ::daemon(1, 0);
}

} // namespace os
