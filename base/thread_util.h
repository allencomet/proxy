#pragma once

#include "data_types.h"
#include "cclog/cclog.h"

#include <time.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <map>
#include <memory>
#include <functional>
#include <iostream>

#include <boost/function.hpp>
#include <boost/bind.hpp>



namespace safe
{
	class Mutex {
	public:
		Mutex() {
			CHECK(pthread_mutex_init(&_mutex, NULL) == 0);
		}

		~Mutex() {
			CHECK(pthread_mutex_destroy(&_mutex) == 0);
		}

		void lock() {
			int err = pthread_mutex_lock(&_mutex);
			CHECK_EQ(err, 0) << ::strerror(err);
		}

		void unlock() {
			int err = pthread_mutex_unlock(&_mutex);
			CHECK_EQ(err, 0) << ::strerror(err);
		}

		bool try_lock() {
			return pthread_mutex_trylock(&_mutex) == 0;
		}

		pthread_mutex_t* mutex() {
			return &_mutex;
		}
	private:
		pthread_mutex_t		_mutex;

		DISALLOW_COPY_AND_ASSIGN(Mutex);
	};

	class condition_variable {
	public:
		condition_variable() {
			CHECK(pthread_cond_init(&_cond, NULL) == 0);
		}

		~condition_variable() {
			CHECK(pthread_cond_destroy(&_cond) == 0);
		}

		void signal() {
			CHECK(pthread_cond_broadcast(&_cond) == 0);
		}

		bool wait(Mutex &mtx) {
			if (0 != pthread_cond_wait(&_cond, mtx.mutex())) {
				return false;
			}
			return true;
		}

		bool timed_wait(Mutex &mtx, uint32 ms);
	private:
		pthread_cond_t _cond;

		DISALLOW_COPY_AND_ASSIGN(condition_variable);
	};

	class RwLock{
	public:
		RwLock(){
			CHECK(pthread_rwlock_init(&_lock, NULL) == 0);
		}

		~RwLock(){
			CHECK(pthread_rwlock_destroy(&_lock) == 0);
		}

		void read_lock(){
			int err = pthread_rwlock_rdlock(&_lock);
			CHECK_EQ(err, 0) << ::strerror(err);
		}

		void write_lock() {
			int err = pthread_rwlock_wrlock(&_lock);
			CHECK_EQ(err, 0) << ::strerror(err);
		}

		void unlock(){
			int err = pthread_rwlock_unlock(&_lock);
			CHECK_EQ(err, 0) << ::strerror(err);
		}

		bool try_read_lock(){
			return pthread_rwlock_tryrdlock(&_lock) == 0;
		}

		bool try_write_lock(){
			return pthread_rwlock_trywrlock(&_lock) == 0;
		}
	private:
		pthread_rwlock_t	_lock;

		DISALLOW_COPY_AND_ASSIGN(RwLock);
	};


	class MutexGuard{
	public:
		explicit MutexGuard(Mutex &mutex):
		_mutex(mutex){
			_mutex.lock();
		}

		~MutexGuard(){
			_mutex.unlock();
		}
	private:
		MutexGuard(const MutexGuard &);
		void operator=(const MutexGuard &);

		Mutex &_mutex;
	};

	class ReadLockGuard{
	public:
		explicit ReadLockGuard(RwLock &rwlock)
			:_rwLock(rwlock){
			_rwLock.read_lock();
		}

		~ReadLockGuard(){
			_rwLock.unlock();
		}
	private:
		ReadLockGuard(const ReadLockGuard &);
		void operator=(const ReadLockGuard &);

		RwLock &_rwLock;
	};

	class WriteLockGuard{
	public:
		explicit WriteLockGuard(RwLock &rwlock)
			:_rwLock(rwlock){
			_rwLock.write_lock();
		}

		~WriteLockGuard(){
			_rwLock.unlock();
		}
	private:
		RwLock &_rwLock;

		DISALLOW_COPY_AND_ASSIGN(WriteLockGuard);
	};


#define atomic_swap __sync_lock_test_and_set
#define atomic_release __sync_lock_release
#define atomic_compare_swap __sync_val_compare_and_swap


	class SpinLock{
	public:
		explicit SpinLock():_locked(false){
		}

		~SpinLock(){}


		bool try_lock(){
			return atomic_swap(&_locked,true) == false;
		}

		void lock(){
			while(!this->try_lock());
		}

		void unlock(){
			atomic_release(&_locked);
		}
	private:
		bool _locked;

		DISALLOW_COPY_AND_ASSIGN(SpinLock);
	};

	class SpinLockGuard{
	public:
		SpinLockGuard(SpinLock &lock)
			:_lock(lock){
			_lock.lock();
		}

		~SpinLockGuard(){
			_lock.unlock();
		}
	private:
		SpinLock& _lock;

		DISALLOW_COPY_AND_ASSIGN(SpinLockGuard);
	};


	class SyncEvent{
	public:
		explicit SyncEvent(bool manual_reset=false,bool signaled=false)
			:_manual_reset(manual_reset),_signaled(signaled){
			pthread_condattr_t	attr;
			CHECK(pthread_condattr_init(&attr) == 0);
			CHECK(pthread_condattr_setclock(&attr,CLOCK_MONOTONIC) == 0);
			CHECK(pthread_cond_init(&_cond,&attr) == 0);
			CHECK(pthread_condattr_destroy(&attr) == 0);
		}

		~SyncEvent(){
			CHECK(pthread_cond_destroy(&_cond) == 0);
		}

		void signal(){
			MutexGuard g(_mutex);
			if (!_signaled){
				_signaled = true;
				pthread_cond_broadcast(&_cond);
			}
		}

		void reset(){
			MutexGuard g(_mutex);
			_signaled = false;
		}

		bool is_signaled(){
			MutexGuard g(_mutex);
			return _signaled;
		}

		void wait(){
			MutexGuard g(_mutex);
			if(!_signaled){
				pthread_cond_wait(&_cond, _mutex.mutex());
				CHECK(_signaled);
			}
			if(!_manual_reset)	_signaled = false;//(如果为自动重置，则将信号重置)
		}

		bool timed_wait(uint32 ms);

	private:
		pthread_cond_t	_cond;
		Mutex _mutex;

		const bool _manual_reset;
		bool	_signaled;

		DISALLOW_COPY_AND_ASSIGN(SyncEvent);
	};

	/************************************************************************/
	/* 
	bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval, ...)
	type __sync_val_compare_and_swap (type *ptr, type oldval type newval, ...)
	These builtins perform an atomic compare and swap. That is, if the current value of *ptr is oldval, 
	then write newval into *ptr.
	*/
	/************************************************************************/
	class Thread{
	public:
		explicit Thread(boost::function<void ()> fun)
		: _fun(fun), _tid(0){
		
		}

		~Thread(){}

		bool start(){
		 if (atomic_compare_swap(&_tid, 0, 1) != 0) return true;
		 return pthread_create(&_tid,NULL,&Thread::thread_fun,&_fun);
		}

		void join(){
			pthread_t id = atomic_swap(&_tid,0);
			if (id != 0) pthread_join(id, NULL);
		}
		
		void detach(){
			pthread_t id = atomic_swap(&_tid,0);
			if (id != 0) pthread_detach(id);
		}

		void cancel(){
			pthread_t id = atomic_swap(&_tid,0);
			if (id != 0) {
				pthread_cancel(id);
				pthread_join(id, NULL);
			}
		}

		uint64 id() const {
			return _tid;
		}

	private:
		boost::function<void ()>	_fun;
		pthread_t	_tid;

		static void *thread_fun(void *p){
			boost::function<void()> f = *((boost::function<void()>*) p);
			f();
			return NULL;
		}

		DISALLOW_COPY_AND_ASSIGN(Thread);
	};


	class StoppableThread : public Thread{
	public:
		explicit StoppableThread(boost::function<void ()> fun,uint32 ms)
			: Thread(boost::bind(&StoppableThread::thread_fun, this)),
			_fun(fun), m_ms(ms), m_stop(false){
		}

		~StoppableThread(){}

		void join(){
			if(atomic_swap(&m_stop, true) == false){
				_ev.signal();
				Thread::join();
			}
		}

		void notify(){
			_ev.signal();
		}

	private:
		SyncEvent _ev;
		boost::function<void ()>	_fun;

		uint32 m_ms;
		bool m_stop;

		void thread_fun(){
			while(!m_stop){
				_ev.timed_wait(m_ms);	//接受到信号执行一次,否则睡眠等待超时后继续执行
				if(!m_stop) _fun();//不关闭的话则执行线程函数
			}
		}

		DISALLOW_COPY_AND_ASSIGN(StoppableThread);
	};

}
