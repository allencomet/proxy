#ifndef __PROXY_CORE_CONNMANAGER_H
#define __PROXY_CORE_CONNMANAGER_H

#include "../../util/util.h"

#include "connection.h"

namespace proxy {

class ConnManager {
  public:
	ConnManager() :_idle_time(60), _max_conn_limit(5000) {}//default idle time is 60 seconds
	~ConnManager() {}

	inline int64 idle_time() const {
		return _idle_time;
	}

	inline void set_idle_time(int64 n) {
		_idle_time = n;
	}

	inline int32 max_conn_limit() const {
		return _max_conn_limit;
	}

	inline void set_max_conn_limit(int32 n) {
		_max_conn_limit = n;
	}

	bool add_conn(int32 fd, ConnectionPtr ptr);

	bool add_conn(int32 fd, const std::string &addr);

	inline void mark_death(int32 fd) {
		safe::MutexGuard g(_kill_fds_mtx);
		_kill_fds.insert(fd);
	}

	void remove_conn(int32 fd);

	ConnectionPtr connection(int32 fd);

	void check_invalid_conn();

  private:
	safe::RwLock _conns_rwmtx;
	boost::unordered_map<int32, ConnectionPtr> _conns;

	safe::Mutex _kill_fds_mtx;
	boost::unordered_set<int32> _kill_fds;

	int64 _idle_time;
	int32 _max_conn_limit;

	DISALLOW_COPY_AND_ASSIGN(ConnManager);
};

}// namespace epollthreadpool


#endif