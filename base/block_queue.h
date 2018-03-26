#ifndef BLOCK_QUEUE_H  
#define BLOCK_QUEUE_H  

#include "base.h"
#include "data_types.h"

#include "thread_util.h"

namespace safe {
	template<class T>
	class block_queue{
	public:
		block_queue(int max_size = 1000){
			if (max_size <= 0){
				exit(-1);
			}

			_max_size = max_size;
			_array = new T[max_size];
			_size = 0;
			_front = -1;
			_back = -1;
		}

		void clear(){
			MutexGuard g(_mutex);
			_size = 0;
			_front = -1;
			_back = -1;
		}

		~block_queue(){
			MutexGuard g(_mutex);
			if (_array != NULL) {
				delete[]_array;
				_array = NULL;
			}
		}

		bool full()const{
			MutexGuard g(_mutex);
			if (_size >= _max_size){
				return true;
			}
			return false;
		}

		bool empty()const{
			MutexGuard g(_mutex);
			if (0 == _size){
				return true;
			}
			return false;
		}

		bool front(T& value)const{
			MutexGuard g(_mutex);
			if (0 == _size){
				return false;
			}
			value = _array[_front];
			return true;
		}

		bool back(T& value)const{
			MutexGuard g(_mutex);
			if (0 == _size){
				return false;
			}
			value = _array[_back];
			return true;
		}

		int size()const{
			uint32 tmp = 0;
			MutexGuard g(_mutex);
			tmp = _size;
			return tmp;
		}

		int max_size()const{
			uint32 tmp = 0;
			MutexGuard g(_mutex);
			tmp = _max_size;
			return tmp;
		}

		bool push(const T& item){
			MutexGuard g(_mutex);
			if (_size >= _max_size){
				_cond.signal();
				return false;
			}

			_back = (_back + 1) % _max_size;
			_array[_back] = item;

			_size++;
			_cond.signal();

			return true;
		}

		bool pop(T& item){
			MutexGuard g(_mutex);
			while (_size <= 0){
				if (!_cond.wait(_mutex)){
					return false;
				}
			}

			_front = (_front + 1) % _max_size;
			item = _array[_front];
			_size--;
			return true;
		}

		bool pop(T& item, uint32 ms_timeout){
			MutexGuard g(_mutex);
			if (_size <= 0){
				if (!_cond.timed_wait(_mutex, ms_timeout)){
					return false;
				}
			}

			if (_size <= 0){
				return false;
			}

			_front = (_front + 1) % _max_size;
			item = _array[_front]; _size--;
			return true;
		}

	private:
		Mutex _mutex;
		condition_variable _cond;
		T *_array;
		uint32 _size;
		uint32 _max_size;
		uint32 _front;
		uint32 _back;

		DISALLOW_COPY_AND_ASSIGN(block_queue);
	};
}

#endif  