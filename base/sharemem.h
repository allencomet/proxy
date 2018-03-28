#pragma once
#include "base.h"
#include "data_types.h"

namespace ipcmem{

	class ShareMem{
	public:
		ShareMem(){}
		ShareMem(int id,int size)
			:m_size(size){
			init(id,size);
		}
		~ShareMem(){
			detach();
			del();
		}

		inline void *addr() const{
			return m_pMem;
		}

		inline int memid() const{
			return m_memid;
		}

		inline bool init(int id,int size){
			m_size = size;
			m_memid = shmget((key_t)id, m_size, 0666|IPC_CREAT); //create shared memory
			if(m_memid == -1) return false; 
			//connecting shared memory to the current process address space
			m_pMem = shmat(m_memid, 0, 0);  
			if(m_pMem == (void*)-1) return false;  
			return true;
		}
	private:
		inline void detach(){
			if (NULL != m_pMem){
				shmdt(m_pMem);
			}
		}

		inline void del(){
			shmctl(m_memid, IPC_RMID, 0);
		}

	private:
		ShareMem(const ShareMem &);
		void operator=(const ShareMem &);

	private:
		void *m_pMem;
		int m_size;
		int m_memid;
	};

}