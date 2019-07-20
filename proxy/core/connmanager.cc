#include "connmanager.h"

namespace proxy {

    ConnectionPtr ConnManager::connection(int32 fd) {
        ConnectionPtr ptr;
        safe::ReadLockGuard g(_conns_rwmtx);
        std::unordered_map<int32, ConnectionPtr>::iterator it = _conns.find(fd);
        if (it != _conns.end()) {
            ptr = it->second;
        }
        return ptr;
    }

    bool ConnManager::add_conn(int32 fd, ConnectionPtr ptr) {
        safe::WriteLockGuard g(_conns_rwmtx);
        if (_conns.size() >= _max_conn_limit) {
            return false;
        }
        _conns.insert(std::make_pair(fd, ptr));
        return true;
    }

    bool ConnManager::add_conn(int32 fd, const std::string &addr) {
        safe::WriteLockGuard g(_conns_rwmtx);
        if (_conns.size() >= _max_conn_limit) {
            return false;
        }
        ConnectionPtr ptr(new proxy::Connection(fd, addr));
        ptr->set_msg_time(sys::utc.ms());//setting up the time of last message
        _conns.insert(std::make_pair(fd, ptr));
        LOG << "the number of current clients: " << _conns.size();
        return true;
    }

    void ConnManager::remove_conn(int32 fd) {
        safe::WriteLockGuard g(_conns_rwmtx);
        std::unordered_map<int32, ConnectionPtr>::iterator it = _conns.find(fd);
        if (it != _conns.end()) {
            WLOG << "close client: fd[" << it->first << "],addr[" << it->second->addr()
                 << "],then remaining " << _conns.size() << " clients";
            ::close(fd);
            _conns.erase(it);
            WLOG << "after close one client,then remaining " << _conns.size() << " clients";
        }
    }

    void ConnManager::check_invalid_conn() {
        //clean idle user
        {
            safe::WriteLockGuard g(_conns_rwmtx);
            for (std::unordered_map<int32, ConnectionPtr>::iterator it = _conns.begin();
                 it != _conns.end();) {
                if (it->second) {
                    if (sys::utc.sec() - it->second->msg_time() > _idle_time) {
                        WLOG << "check client idle timeout: fd[" << it->first << "],addr[" << it->second->addr()
                             << "]...";
                        ::close(it->second->fd());
                        _conns.erase(it++);
                    } else {
                        ++it;
                    }
                } else {
                    _conns.erase(it++);
                }
            }
        }

        //clean invalid user
        safe::MutexGuard g(_kill_fds_mtx);
        for (std::unordered_set<int32>::iterator it = _kill_fds.begin();
             it != _kill_fds.end(); ++it) {
            LOG << "remove client " << *it;
            remove_conn(*it);
        }
        _kill_fds.clear();
    }

}//namespace epollthreadpool