#include "os.h"
#include "cclog/cclog.h"
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>

namespace os {

std::string system(const std::string& cmd) {
    int fd[2];
    if (pipe(fd) != 0) {
		ELOG << "create pipe failed..";
        return std::string();
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        dup2(fd[1], STDERR_FILENO);

        int ret = execl("/bin/bash", "bash", "-c", cmd.c_str(), (char*) NULL);
        if (ret == -1) {
			ELOG << "execl failed: " << cmd << ", err: " << strerror(errno);
            return std::string();
        }
    }

    close(fd[1]);
    waitpid(pid, NULL, 0);

    std::string ret;
    char buf[4096];

    fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL, 0) | O_NONBLOCK);
    for (ssize_t len = 0;;) {
        len = read(fd[0], buf, 4096);
        if (len > 0) ret.append(buf, len);
        if (len < 4096) break;
    }

    fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL, 0) & ~O_NONBLOCK);
    close(fd[0]);
    return ret;
}

bool rmdirs(const std::string& path){
	DIR * pDir = NULL;  
	struct dirent *pDirent = NULL; 

	if((pDir = opendir(path.c_str())) == NULL){
		return FALSE;
	}

	char szBuf[256] = {0};
	while((pDirent = readdir(pDir)) != NULL){
		if(strcmp(pDirent->d_name,".") == 0 
			|| strcmp(pDirent->d_name,"..") == 0){
				continue;
		}

		if(pDirent->d_type == DT_DIR){
			memset(szBuf,0,256);
			snprintf(szBuf,256,"%s/%s",path.c_str(),pDirent->d_name);
			rmdirs(szBuf);
		}else{
			memset(szBuf,0,256);
			snprintf(szBuf,256,"%s/%s",path.c_str(),pDirent->d_name);
			remove(szBuf);//delete file
		}
	}

	closedir(pDir);

	if(0 != remove(path.c_str())){
		return false;
	}

	return true;
}

std::vector<std::string> readdir(const std::string& path) {
    std::vector<std::string> v;

    DIR* dir = ::opendir(path.c_str());
    if (dir == NULL) return v;

    struct dirent* ent = NULL;
    while ((ent = ::readdir(dir)) != NULL) {
        if (ent->d_name[0] != '.') v.push_back(ent->d_name);
    }

    ::closedir(dir);
    return v;
}

bool file::open(const std::string& path, const std::string& mode) {
    this->close();

    if (!path.empty()) _path = path;
    if (!mode.empty()) _mode = mode;

    _fd = ::fopen(_path.c_str(), mode.c_str());
    if (_fd == NULL) return false;

    _mtime = os::path.mtime(_path);
    _eof = false;
    _lineno = 0;

    return true;
}

void file::close() {
    if (_fd != NULL) {
        ::fclose(_fd);
        _fd = NULL;
    }
}

std::string file::getline() {
    char* line = NULL;
    size_t len = 0;

    size_t r = ::getline(&line, &len, _fd);
    r < 0 ? _eof = true : ++_lineno;

    std::string s;
    if (r > 0) s = std::string(line, line[r - 1] == '\n' ? r - 1 : r);
    if (line != NULL) ::free(line);

    return s;
}

std::vector<std::string> file::getlines() {
    return str::split(this->read(), '\n');
}

std::string file::read(uint32 size) {
    if (size == -1) {
        size = (int32) this->size();
        CHECK_NE(size, -1);
    }

    std::string s;
    s.resize(size);

    size_t r = ::fread(&s[0], 1, s.size(), _fd);
    CHECK_GE(r, 0);

    if (r < s.size()) {
        _eof = true;
        s.resize(r);
    }

    return s;
}
} // namespace os
