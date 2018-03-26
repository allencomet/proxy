#include "ipc.h"


namespace{
	bool sock_unix_pair(int &lfd,int &rfd){
		int fdPair[2] = {0};
		if (0 == socketpair(PF_UNIX,SOCK_STREAM,0,fdPair)){
			lfd = fdPair[0];
			rfd	= fdPair[1];
			return true;
		}else{
			return false;
		}
	}

	

	class Safe_StreamBuf{
	public:
		Safe_StreamBuf(){

		}

		~Safe_StreamBuf(){}

	private:
		Safe_StreamBuf(const Safe_StreamBuf &);
		void operator=(const Safe_StreamBuf &);

	private:
		StreamBuf _buf;
	};
}


namespace core{
	namespace ipc{

		struct LoHead{
			char	m_szUserID[16];	//用户ID
			int32	m_nMsgType;		//消息类型(0:进程退出,1:业务消息,2:主进程检测到交易日更新)
			int32	m_nFlowNo;		//流水号
			uint32	m_nFuncNo;		//功能号
			int32	m_nErrNo;		//错误码(请求包设置0)
			uint32	m_nDataLen;		//数据长度
		};

		static void show_head(LoHead *pHead){
			std::cout << "UserID: " << pHead->m_szUserID << ",MsgType: " << pHead->m_nMsgType
				<< ",flow: " << pHead->m_nFlowNo << ",function: " << pHead->m_nFuncNo
				<< ",errno: " << pHead->m_nErrNo << ",DataLen: " << pHead->m_nDataLen << std::endl;
		}

		//create socket pair
		void LocalIPC::init(){
			int lfd = 0;
			int rfd = 0;
			for (int i = 0; i < m_nCount; ++i){
				if (sock_unix_pair(lfd,rfd)){
					m_mSock.insert(std::make_pair(lfd,rfd));
				}
			}
		}

		//Responsible for receive and send package
		class process{
		public:
			explicit process(int nCount)
				:m_localIPC(nCount){
			}

			~process(){}

			inline void setpid(int pid){
				m_pid = pid;
			}

			//close the other descriptor,preserve one descriptor in child
			inline void init_child(){
				m_localIPC.init_child();
			}

			//close the other descriptor,preserve one descriptor in parent
			inline void init_parent(){
				m_localIPC.init_parent();
			}

			//parent and child both run here
			void run();	

		private:
			process(const process &);
			void operator=(const process &);

			void register_fd();
			void on_receive(int fd);
			void on_send(int fd);
			void on_error(int fd);

			void read_head(int fd,StreamBuf *prbuf);
			void read_body(int fd,StreamBuf *prbuf);

			void parsing_rbuf(StreamBuf *);

			inline int body_len(LoHead *pHead){
				return pHead->m_nDataLen;
			}

			inline void regester_read(int32 fd){
				epoll_event ev;
				ev.data.fd = fd;
				ev.events = EPOLLIN | EPOLLONESHOT;
				epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &ev);
			}

			inline void regester_write(int32 fd){
				epoll_event ev;
				ev.data.fd = fd;
				ev.events = EPOLLOUT | EPOLLONESHOT;
				epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &ev);
			}
		private:
			const static int MAX_EVENT = 64;
			const static int HEAD_LEN = sizeof(LoHead);//initialize header length
			const static int MAX_BODY_LEN = LocalIPC::BUFF_LENGTH - sizeof(LoHead);//initialize header length
			
			int m_pid;		//process id
			int32 m_epoll;	//epoll fd
			LocalIPC m_localIPC;
		};

		void process::register_fd(){
			std::set<int> &fds = m_localIPC.get_socks();
			for (std::set<int>::iterator it = fds.begin(); 
				it != fds.end(); ++it){
					epoll_event ev;
					ev.data.fd = *it;
					ev.events = EPOLLIN | EPOLLONESHOT;
					epoll_ctl(m_epoll, EPOLL_CTL_MOD, *it, &ev);
			}
		}

		void process::parsing_rbuf(StreamBuf *prbuf){
			LoHead* pMsgHead = (LoHead *)prbuf->data();
			
			show_head(pMsgHead);
			
			const char *pBody = (const char *)(prbuf->data() + sizeof(LoHead));

			std::cout << "receive content: " << pBody << std::endl;

			return;
		}

		void process::read_head(int fd,StreamBuf *prbuf){
			int32 nCount = 0;
			char szBuffer[HEAD_LEN];
			nCount = recv(fd,szBuffer,HEAD_LEN,0);
			if(nCount < 0) {//occur error
				if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)  {
					regester_read(fd);
				}else{
					std::cerr << "process[" << m_pid << "] failed to read [" << fd << "],error msg["
						<< errno << ";" << strerror(errno) << "]" << std::endl;
				}
			}else if(nCount == 0){
				std::cerr << "closed by peer" << std::endl;
			}else{
				prbuf->append(szBuffer,nCount);
				if (nCount == HEAD_LEN){//read a complete header
					prbuf->set_body_len(body_len((LoHead *)prbuf->data()));
					read_body(fd,prbuf);
				}
				regester_read(fd);
			}
		}

		void process::read_body(int fd,StreamBuf *prbuf){
			int32 nCount = 0;
			int body_len = prbuf->bodylen();
			char *pTmpBuf = new char[body_len+1];
			memset(pTmpBuf,0,body_len+1);
			nCount = recv(fd,pTmpBuf,body_len,0);
			if(nCount < 0) {//读取时有错误
				if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)  {
					regester_read(fd);
				}else{
					std::cerr << "process[" << m_pid << "] failed to read [" << fd << "],error msg["
						<< errno << ";" << strerror(errno) << "]" << std::endl;
				}
			}else if(nCount == 0){
				std::cerr << "closed by peer" << std::endl;
			}else{
				prbuf->append(pTmpBuf,nCount);
				if (nCount == body_len){
					parsing_rbuf(prbuf);	//invoke handler here
					prbuf->mark_read_header(false);//continue to reading header next time
					prbuf->set_body_len(0);
				}
				regester_read(fd);
			}

			delete[] pTmpBuf;
			pTmpBuf = NULL;
		}

		void process::on_receive(int fd){
			StreamBuf* prbuf = m_localIPC.sock_rbuffer(fd);

			if (prbuf->header_has_read()){
				read_body(fd,prbuf);
			}else{
				read_head(fd,prbuf);
			}
		}

		void process::on_send(int fd){
			StreamBuf* prbuf = m_localIPC.sock_wbuffer(fd);


		}

		void process::on_error(int fd){
			
		}	

		void process::run(){
			m_epoll = epoll_create(MAX_EVENT);
			if(m_epoll < 0){
				std::cerr << "create epoll failed [" << errno 
					<< ":" << strerror(errno) << "]" << std::endl; 
				exit(EXIT_FAILURE);
			}

			register_fd();
			
			std::cout << "running in process [" << m_pid
				<< "]" << std::endl;

			epoll_event events[MAX_EVENT];
			int32 nCount = 0;
			//循环等待epoll事件的发生
			for(;;){
				nCount = 0;
				pthread_testcancel();
				nCount = epoll_wait(m_epoll, events, MAX_EVENT, 10);
				pthread_testcancel();

				//处理所发生的所有事件
				for (int32 i = 0; i < nCount; i++){
					if (events[i].events & EPOLLIN){
						on_receive(events[i].data.fd);
					}else if (events[i].events & EPOLLOUT ){
						on_send(events[i].data.fd);
					}else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP){
						on_error(events[i].data.fd);
					}
				}
			}
		}

		class process_pool{
		public:
			explicit process_pool(int nPCount,int nSockCount)
				:m_nPCount(nPCount),m_nSockCount(nSockCount),m_nIndex(-1){
					init();
			}

			~process_pool(){
				uninstall_parent_handler();
				uninstall_child_handler();
			}

			void install_parent_handler();
			void uninstall_parent_handler();

			void install_child_handler();
			void uninstall_child_handler();

			inline void run(){
				if (-1 == m_nIndex){
					run_parent();
				}else{
					run_child();
				}
			}

		private:
			process_pool(const process_pool &);
			void operator=(const process_pool &);

			static void* thread_fun(void *pParam);

			void init();

			void run_child();
			void run_parent();
		private:
			int m_nIndex;	//mark indicate index 
			int m_nPCount;
			int m_nSockCount;
			std::map<int,process *>	m_mProcess;
			std::map<int,pthread_t>	m_mThreaddID;
		};

		void install_parent_handler(){
		
		}

		void uninstall_parent_handler(){
		
		}

		void install_child_handler(){
		
		}

		void uninstall_child_handler(){
		
		}

		void process_pool::init(){
			int pid = 0;
			for (int i = 0; i < m_nPCount; ++i){
				process *pPro = new process(m_nSockCount);//create socket pair while crate process
				pid = fork();
				if (0 < pid	){//parent
					pPro->setpid(pid);
					pPro->init_parent();
					m_mProcess.insert(std::make_pair(pid, pPro));//process pool in parent 
				}else if (0 == pid){//child
					m_nIndex = i;//setting index of subprocess
					pPro->setpid(getpid());
					pPro->init_child();
					m_mProcess.insert(std::make_pair(pid, pPro));//process pool in child
					break;
				}else{//failed
					delete pPro;
					pPro = NULL;
					exit(EXIT_FAILURE);
				}
			}
		}

		//thread function: receiving and parsing the front request package 
		void* process_pool::thread_fun(void *pParam){
			if(NULL == pParam) return NULL;
			process *pPro = (process *)pParam;

			pPro->run();	//running subprocess

			return NULL;
		}

		//running in child process(single process)
		void process_pool::run_child(){
			int pid = getpid();
			pthread_t tid;
			std::map<int,process *>::iterator it = m_mProcess.find(pid);
			if (it != m_mProcess.end()){
				pthread_create(&tid,NULL,thread_fun,it->second);
			}
			
			if (tid != 0) pthread_join(tid, NULL);
		}

		//running in parent process
		void process_pool::run_parent(){
			std::vector<pthread_t>	tids;
			pthread_t tid;

			//create thread for receiving and parsing front request package
			for (std::map<int,process *>::iterator it = m_mProcess.begin();
				it != m_mProcess.end(); ++it){
					pthread_create(&tid,NULL,thread_fun,it->second);
					tids.push_back(tid);
			}

			for (std::vector<pthread_t>::iterator it = tids.begin();
				it != tids.end(); ++it){
					if (*it != 0) pthread_join(*it, NULL);
			}
		}

	}//namespace ipc
}//namespace core



namespace test{
	class ClxTest{
	public:
		ClxTest():m_iTimes(0){}
		~ClxTest(){}
		void Output() const{
			std::cout << "Output for test!" << std::endl;
			m_iTimes++;//m_iTimes被mutable修饰，可以突破const的限制
		}
		int GetOutputTimes() const{
			return m_iTimes;
		}
	private:
		mutable int m_iTimes;
	}; 
}


