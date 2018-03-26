#pragma once

#include "../util/util.h"


namespace core{
	namespace ipc{

class LocalIPC{
public:
	enum PROCESSTYPE{
		UNKNOW,
		CHILD,
		PARENT
	};

	const static int BUFF_LENGTH  =  8*1024;//Max read buffer
public:
	explicit LocalIPC(int nCount)
		:m_nCount(nCount),m_type(UNKNOW){
			init();
	}

	~LocalIPC(){
		uninit();
	}
	
	//initialize child process
	inline void init_child(){
		m_type = CHILD;
		for (std::map<int,int>::iterator it = m_mSock.begin();
			it != m_mSock.end(); ++it){
				close(it->first);
				m_sChild.insert(it->second);
				StreamBuf *prbuf = new StreamBuf(BUFF_LENGTH);
				m_mRBuf.insert(std::make_pair(it->second,prbuf));
				StreamBuf *pwbuf = new StreamBuf(BUFF_LENGTH);
				m_mWBuf.insert(std::make_pair(it->second,pwbuf));
		}
		m_mSock.clear();
	}

	//initialize parent process
	inline void init_parent(){
		m_type = PARENT;
		for (std::map<int,int>::iterator it = m_mSock.begin();
			it != m_mSock.end(); ++it){
				close(it->second);
				m_sChild.insert(it->first);
				StreamBuf *prbuf = new StreamBuf();
				m_mRBuf.insert(std::make_pair(it->first,prbuf));
				StreamBuf *pwbuf = new StreamBuf();
				m_mWBuf.insert(std::make_pair(it->first,pwbuf));
		}
		m_mSock.clear();
	}

	inline bool is_child(){
		return m_type == CHILD ? true : false;
	}

	inline bool is_parent(){
		return m_type == PARENT ? true : false;
	}

	inline PROCESSTYPE type(){
		return m_type;
	}

	//get all of its own socket descriptors
	inline std::set<int>& get_socks(){
		if (CHILD == m_type){
			return child_socks();
		}else if (PARENT == m_type){
			return parent_socks();
		}
	}

	//read buffer
	inline StreamBuf* sock_rbuffer(int fd){
		std::map<int,StreamBuf *>::iterator it = m_mRBuf.find(fd);
		if (m_mRBuf.end() != it){
			return it->second;
		}else{
			return NULL;
		}
	}

	//write buffer
	inline StreamBuf* sock_wbuffer(int fd){
		std::map<int,StreamBuf *>::iterator it = m_mWBuf.find(fd);
		if (m_mWBuf.end() != it){
			return it->second;
		}else{
			return NULL;
		}
	}
private:
	LocalIPC(const LocalIPC &);
	void operator=(const LocalIPC &);

	inline std::set<int>& parent_socks(){
		return m_sParent;
	}

	inline std::set<int>& child_socks(){
		return m_sChild;
	}

	//create socket pair
	void init();
	void uninit(){
		for (std::map<int,StreamBuf *>::iterator it = m_mRBuf.begin();
			it != m_mRBuf.end(); ++it){
				if (NULL != it->second){
					delete it->second;
					it->second = NULL;
				}
				close(it->first);
		}
		m_mRBuf.clear();
		
		for (std::map<int,StreamBuf *>::iterator it = m_mWBuf.begin();
			it != m_mWBuf.end(); ++it){
				if (NULL != it->second){
					delete it->second;
					it->second = NULL;
				}
				close(it->first);
		}
		m_mWBuf.clear();
	}
private:
	std::set<int>	m_sParent;
	std::set<int>	m_sChild;
	std::map<int,StreamBuf *>	m_mRBuf;//read buffer
	std::map<int,StreamBuf *>	m_mWBuf;//write buffer
	std::map<int,int>	m_mSock;//socket pair
	int m_nCount;
	PROCESSTYPE		m_type;
};


	}//namespace ipc
}//namespace core