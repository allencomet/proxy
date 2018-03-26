#include "evhandler.h"

bool evhandler::timed_wait(int32 flow, uint32 ms) {
	SyncEventPtr ev;
	{
		safe::MutexGuard g(_flow2ev_mtx);
		boost::unordered_map<int32, SyncEventPtr>::iterator it = _flow2ev.find(flow);//流水号对应的事件信号量
		if (_flow2ev.end() != it) {
			ev = it->second;
		}else {
			errorlog::err_msg("can not find flow %d",flow);
		}
	}
	if (ev){
		return ev->timed_wait(ms);
	}else {
		errorlog::err_msg("SyncEventPtr is NULL");
		return false;
	}
}

void evhandler::signal(int32 flow) {
	safe::MutexGuard g(_flow2ev_mtx);
	boost::unordered_map<int32, SyncEventPtr>::iterator it = _flow2ev.find(flow);//流水号对应的事件信号量
	if (_flow2ev.end() != it) {
		(it->second)->signal();
	}
}