#pragma once

#include "base.h"
#include "data_types.h"

namespace safe {

    class ShareMem {
    public:
        ShareMem() {}

        ShareMem(int id, int size)
                : _size(size) {
            init(id, size);
        }

        ~ShareMem() {
            detach();
            del();
        }

        inline void *addr() const {
            return _pMem;
        }

        inline int memid() const {
            return _mem_id;
        }

        inline bool init(int id, int size) {
            _size = size;
            _mem_id = shmget((key_t) id, _size, 0666 | IPC_CREAT); //create shared memory
            if (_mem_id == -1) return false;
            //connecting shared memory to the current process address space
            _pMem = shmat(_mem_id, 0, 0);
            if (_pMem == (void *) -1) return false;
            return true;
        }

    private:
        inline void detach() {
            if (NULL != _pMem) {
                shmdt(_pMem);
            }
        }

        inline void del() {
            shmctl(_mem_id, IPC_RMID, 0);
        }

    private:
        void *_pMem;
        int _size;
        int _mem_id;

        DISALLOW_COPY_AND_ASSIGN(ShareMem);
    };


    /*union semun {
        int32 val;
        struct semid_ds *buf;
        unsigned short *arry;
    };*/

    class SemMutex {
    public:
        SemMutex() {}

        SemMutex(int id) {
            init_semvalue(id);
        }

        ~SemMutex() {
            del_semvalue(_sem_id);
        }

        inline bool init(int id) {
            return init_semvalue(id);
        }

        inline bool lock() {
            return semaphore_p(_sem_id);
        }

        inline bool unlock() {
            return semaphore_v(_sem_id);
        }

    private:
        union semun {
            int32 val;
            struct semid_ds *buf;
            unsigned short *arry;
        };

        inline bool init_semvalue(int id) {
            _sem_id = semget((key_t) id, 1, 0666 | IPC_CREAT);//create semaphore
            return set_semvalue(_sem_id);
        }

        inline bool set_semvalue(int32 sem_id) {
            union semun sem_union;  //initialize semaphore before use it
            sem_union.val = 1;
            if (semctl(sem_id, 0, SETVAL, sem_union) == -1)
                return false;
            return true;
        }

        inline bool del_semvalue(int sem_id) {
            union semun sem_union;
            if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1) {
                return false;
            }
            return true;
        }

        //waitting semaphore is available
        inline bool semaphore_p(int sem_id) {
            struct sembuf sem_b;
            sem_b.sem_num = 0;
            sem_b.sem_op = -1;//P()
            sem_b.sem_flg = SEM_UNDO;
            if (semop(sem_id, &sem_b, 1) == -1) {
                return false;
            }
            return true;
        }

        //release action,makes semaphore enable,send V signal
        inline bool semaphore_v(int sem_id) {
            struct sembuf sem_b;
            sem_b.sem_num = 0;
            sem_b.sem_op = 1;//V()
            sem_b.sem_flg = SEM_UNDO;
            if (semop(sem_id, &sem_b, 1) == -1) {
                return false;
            }
            return true;
        }

    private:
        int _sem_id;

        DISALLOW_COPY_AND_ASSIGN(SemMutex);
    };

    class SemLockGuard {
    public:
        explicit SemLockGuard(SemMutex &mutex)
                : _sem_mtx(mutex) {
            _sem_mtx.lock();
        }

        ~SemLockGuard() {
            _sem_mtx.unlock();
        }

    private:
        SemMutex &_sem_mtx;

        DISALLOW_COPY_AND_ASSIGN(SemLockGuard);
    };
}
