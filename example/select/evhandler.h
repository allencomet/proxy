#ifndef __EV_HANDLER_H
#define __EV_HANDLER_H

#include "../util/util.h"

class evhandler {
public:
	evhandler() {}
	~evhandler() {}


	void install_ev_handler(int32 flow) {
		SyncEventPtr ev(new safe::SyncEvent());
		safe::MutexGuard g(_flow2ev_mtx);
		_flow2ev.insert(std::make_pair(flow, ev));
	}

	void uninstall_ev_handler(int32 flow) {
		safe::MutexGuard g(_flow2ev_mtx);
		_flow2ev.erase(flow);
	}

	bool timed_wait(int32 flow, uint32 ms);

	void signal(int32 flow);
private:
	typedef boost::shared_ptr<safe::SyncEvent> SyncEventPtr;
	safe::Mutex _flow2ev_mtx;
	boost::unordered_map<int32, SyncEventPtr> _flow2ev;//流水号对应的事件信号量

	DISALLOW_COPY_AND_ASSIGN(evhandler);
};

#endif