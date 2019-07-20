#ifndef __PROXY_CORE_CONNMAP_H
#define __PROXY_CORE_CONNMAP_H

#include "../../util/util.h"

#include "connmanager.h"

namespace proxy {

    class Connmap {
    public:
        Connmap(ConnManager &f, ConnManager &b) : _front(f), _back(b) {}

        ~Connmap() {}

        //find back connection by front fd
        ConnectionPtr back_connptr(int32 frontfd) {
            ConnectionPtr connptr;
            safe::ReadLockGuard g(_fd_map_mtx);
            std::unordered_map<int32, int32>::iterator it = _front2back_fd_map.find(frontfd);
            if (it != _front2back_fd_map.end()) {
                connptr = _back.connection(it->second);
            }
            return connptr;
        }

        //find front connection by back fd
        ConnectionPtr front_connptr(int32 backfd) {
            ConnectionPtr connptr;
            safe::ReadLockGuard g(_fd_map_mtx);
            std::unordered_map<int32, int32>::iterator it = _back2front_fd_map.find(backfd);
            if (it != _back2front_fd_map.end()) {
                connptr = _front.connection(it->second);
            }
            return connptr;
        }

        void associate(int32 front, int32 back) {
            safe::WriteLockGuard g(_fd_map_mtx);
            _front2back_fd_map.insert(std::make_pair(front, back));
            _back2front_fd_map.insert(std::make_pair(back, front));
        }

        void unassociate_by_front(int32 frontfd) {
            safe::WriteLockGuard g(_fd_map_mtx);
            std::unordered_map<int32, int32>::iterator it = _front2back_fd_map.find(frontfd);
            if (it != _front2back_fd_map.end()) {
                std::unordered_map<int32, int32>::iterator it2 = _back2front_fd_map.find(it->second);
                if (it2 != _back2front_fd_map.end()) {
                    _back.remove_conn(it2->first);
                    _back2front_fd_map.erase(it2);
                }
                _front.remove_conn(it->first);
                _front2back_fd_map.erase(it);
            }
        }

        void unassociate_by_back(int32 backfd) {
            safe::WriteLockGuard g(_fd_map_mtx);
            std::unordered_map<int32, int32>::iterator it = _back2front_fd_map.find(backfd);
            if (it != _back2front_fd_map.end()) {
                std::unordered_map<int32, int32>::iterator it2 = _front2back_fd_map.find(it->second);
                if (it2 != _front2back_fd_map.end()) {
                    _front.remove_conn(it2->first);
                    _front2back_fd_map.erase(it2);
                }
                _back.remove_conn(it->first);
                _back2front_fd_map.erase(it);
            }
        }

    private:
        ConnManager &_front;
        ConnManager &_back;

        safe::RwLock _fd_map_mtx;
        std::unordered_map<int32, int32> _front2back_fd_map;
        std::unordered_map<int32, int32> _back2front_fd_map;

        DISALLOW_COPY_AND_ASSIGN(Connmap);
    };

    typedef std::shared_ptr<Connmap> ConnmapPtr;

}


#endif