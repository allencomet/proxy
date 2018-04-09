#ifndef __NET_MT_LB_LOADBALANCER_H
#define __NET_MT_LB_LOADBALANCER_H

#include "../../util/util.h"

namespace lb {

class Server {
public:
	Server(const std::string str, int n, int w = 0)
		:ip(str),
		port(n),
		weight(w),
		id(0) {
	}

	std::string ip;
	int port;
	int weight;
	int id;
};

class LBAlgo {
public:
	LBAlgo() {}
	~LBAlgo() {}

	virtual int size() = 0;
	virtual bool empty() = 0;

	virtual Server *get() = 0;
	virtual void add(Server *) = 0;
	virtual void del(Server *) = 0;
};


class RandomAlog : public LBAlgo {
public:
	RandomAlog() {
		srand(time(NULL));
	}

	~RandomAlog() {
	}

	virtual int size() {
		return _servers.size();
	}

	virtual bool empty() {
		return _servers.empty();
	}

	virtual Server *get() {
		return _servers[rand() % _servers.size()];
	}

	virtual void add(Server *ptr) {
		if (!ptr) return;
		_servers.push_back(ptr);
	}

	virtual void del(Server *ptr) {
		for (std::vector<Server *>::iterator it = _servers.begin();
			it != _servers.end(); ++it) {
			if ((*it)->id == ptr->id) {
				_servers.erase(it);
			}
		}
	}

private:
	std::vector<Server *> _servers;
};


class RandomWeightAlog : public LBAlgo {
public:
	RandomWeightAlog() {
		srand(time(NULL));
	}

	~RandomWeightAlog() {}

	virtual int size() {
		return _servers_array.size();
	}

	virtual bool empty() {
		return _servers_array.empty();
	}

	virtual Server *get() {
		return _servers_array[rand() % _servers_array.size()];
	}

	virtual void add(Server *ptr) {
		if (!ptr) return;

		int w = ptr->weight;
		for (int i = 0; i < w; ++i)
			_servers_array.push_back(ptr);
	}


	virtual void del(Server *ptr) {
		for (std::vector<Server *>::iterator it = _servers_array.begin();
			it != _servers_array.end();) {
			if ((*it)->id == ptr->id) {
				it = _servers_array.erase(it);
			} else {
				++it;
			}
		}
	}
private:
	std::vector<Server *> _servers_array;
};


class RoundRobbinAlgo : public LBAlgo {
public:
	RoundRobbinAlgo() :_index(0) {}
	~RoundRobbinAlgo() {}

	virtual int size() {
		return _servers_array.size();
	}

	virtual bool empty() {
		return _servers_array.empty();
	}

	virtual Server *get() {
		_index = _index++%_servers_array.size();
		return _servers_array[_index];
	}

	virtual void add(Server *ptr) {
		if (ptr) _servers_array.push_back(ptr);
	}

	virtual void del(Server *ptr) {
		for (std::vector<Server *>::iterator it = _servers_array.begin();
			it != _servers_array.end(); ++it) {
			if ((*it)->id == ptr->id) {
				_servers_array.erase(it);
			}
		}
	}

private:
	std::vector<Server *> _servers_array;
	int _index;
};


class RoundRobbinWeightAlgo : public LBAlgo {
public:
	RoundRobbinWeightAlgo() :_index(0) {}

	virtual int size() {
		return _servers_array.size();
	}

	virtual bool empty() {
		return _servers_array.empty();
	}

	virtual Server *get() {
		_index = _index++%_servers_array.size();
		return _servers_array[_index];
	}

	virtual void add(Server *ptr) {
		if (!ptr) return;

		int w = ptr->weight;
		for (int i = 0; i < w; ++i)
			_servers_array.push_back(ptr);
	}

	virtual void del(Server *ptr) {
		for (std::vector<Server *>::iterator it = _servers_array.begin();
			it != _servers_array.end();) {
			if ((*it)->id == ptr->id) {
				it = _servers_array.erase(it);
			} else {
				++it;
			}
		}
	}

private:
	std::vector<Server *> _servers_array;
	int _index;
};


class HashAlgo : public LBAlgo {
public:
	HashAlgo(boost::function<int(int)> fun) :_fun(fun) {}
	~HashAlgo() {}

	virtual int size() {
		return _servers_array.size();
	}

	virtual bool empty() {
		return _servers_array.empty();
	}

	virtual Server *get(int id) {
		if (_fun) {
			return _servers_array[_fun(id)];
		} else {
			return _servers_array[hash_algo(id)];
		}
	}

	virtual void add(Server *ptr) {
		if (ptr) _servers_array.push_back(ptr);
	}

	virtual void del(Server *ptr) {
		for (std::vector<Server *>::iterator it = _servers_array.begin();
			it != _servers_array.end(); ++it) {
			if ((*it)->id == ptr->id) {
				_servers_array.erase(it);
			}
		}
	}

private:
	int hash_algo(int id) {
		return id%_servers_array.size();
	}

	std::vector<Server *> _servers_array;
	boost::function<int(int)>	_fun;
};


class HashWeightAlgo : public LBAlgo {
public:
	HashWeightAlgo(boost::function<int(int)> fun) :_fun(fun) {}
	~HashWeightAlgo() {}


	virtual int size() {
		return _servers_array.size();
	}

	virtual bool empty() {
		return _servers_array.empty();
	}

	virtual Server *get(int id) {
		if (_fun) {
			return _servers_array[_fun(id)];
		} else {
			return _servers_array[hash_algo(id)];
		}
	}

	virtual void add(Server *ptr) {
		if (!ptr) return;

		int w = ptr->weight;
		for (int i = 0; i < w; ++i)
			_servers_array.push_back(ptr);
	}

	virtual void del(Server *ptr) {
		for (std::vector<Server *>::iterator it = _servers_array.begin();
			it != _servers_array.end();) {
			if ((*it)->id == ptr->id) {
				it = _servers_array.erase(it);
			} else {
				++it;
			}
		}
	}

private:
	int hash_algo(int id) {
		return id%_servers_array.size();
	}

	std::vector<Server *> _servers_array;
	boost::function<int(int)>	_fun;
};

//半成品，后续完善
class ConsistentHashAlgo : public LBAlgo {
	ConsistentHashAlgo(boost::function<int(int)> fun) :_fun(fun) {}
	~ConsistentHashAlgo() {}

	virtual int size() {
		return _group.size();
	}

	virtual bool empty() {
		return _group.empty();
	}

	virtual Server *get(int id) {

	}

	virtual void add(Server *ptr) {
		if (!ptr) return;

		_group.insert(std::make_pair(hash_algo(ptr->id), ptr));
	}

	virtual void del(Server *ptr) {
		_group.erase(hash_algo(ptr->id));
	}
private:
	int hash_algo(int id) {
		return id % 0x7FFFFFFF;
	}

	//0 ~ (2^32) - 1 
	std::map<int, Server *> _group;//key: hash value, value: BServer


	boost::function<int(int)>	_fun;
};


template<typename Algo,typename Server>
class LoadBalancer {
	LoadBalancer() {}
	~LoadBalancer() {}

	void add(Server *srv) {
		_algo.add(srv);
	}

	void del(Server *srv) {
		_algo.del(srv);
	}

	Server *getServer() {
		return _algo.get();
	}

private:
	Algo _algo;

	DISALLOW_COPY_AND_ASSIGN(LoadBalancer);
};

}

/************************************************************************/
/* 
usage:
----------------------------
Server *srv1 = new Server("192.168.0.1",8080);
Server *srv2 = new Server("192.168.0.2",8080);
Server *srv3 = new Server("192.168.0.3",8080);

LoadBalancer<RandomAlog,Server> lb;
lb.add(srv1);
lb.add(srv2);
lb.add(srv3);

//there's a request I have reserved,so I need to choose a server from the pool of server I've setting by specify algorithm
handle(lb.get());

*/
/************************************************************************/


#endif