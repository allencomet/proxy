#pragma once

#include "data_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>


class StreamBuf {
  public:
    explicit StreamBuf(uint32 size = 32):_rhead(false),_bodylen(0) {
        _pbeg = (char*) ::malloc(size);	
        _pcur = _pbeg;
        _pend = _pbeg + size;
    }

    ~StreamBuf() {
        ::free(_pbeg);
    }

    uint32 size() const {
        return static_cast<uint32>(_pcur - _pbeg);
    }

    uint32 capacity() const {
        return static_cast<uint32>(_pend - _pbeg);		//return capacity
    }

    bool empty() const {
        return _pcur == _pbeg;
    }

    // not null-terminated
    const char* data() const {
        return _pbeg;
    }

	const char *data(uint32 n) const{
		return _pbeg + n;
	}

    const char *head() const {
        return _pbeg;
    }

    const char *tail() const {
        return _pcur;
    }

    void addoffset(uint32 n){
        _pcur += n;
    }

    std::string to_string() const {
        return std::string(this->data(), this->size());
    }

    void clear() {
        _pcur = _pbeg;
    }

	void clear_head(uint32 n){
        uint32 size = this->size();
		uint32 count = size - n;
		if (0 < count){
			::memmove(_pbeg, _pbeg + n, count);
			_pcur -= n;
			//::memset(_pcur, '\0', n);
		}else {
			_pcur = _pbeg;
		}
	}

	void clear_tail(uint32 n){
        _pcur -= n;
	}

    void resize(uint32 n) {
        uint32 size = this->size();
        if (n > size) ::memset(_pcur, 0, n - size);
        _pcur = _pbeg + n;
    }

    bool reserve(uint32 n) {
        uint32 size = this->size();
        if (n <= size) return true;

        char* p = (char*) ::realloc(_pbeg, n);
        if (p == NULL) return false;

        _pbeg = p;
        _pcur = p + size;
        _pend = p + n;

        return true;
    }

    StreamBuf& append(const void* data, uint32 size) {
        if (static_cast<uint32>(_pend - _pcur) < size) {
            if (!this->reserve(this->capacity() + size + 32)) {
                return *this;
            }
        }

        ::memcpy(_pcur, data, size);
        _pcur += size;
        return *this;
    }

    StreamBuf& operator<<(bool v) {
        return v ? this->append("true", 4) : this->append("false", 5);
    }

    StreamBuf& operator<<(char v) {
        return this->append(&v, 1);
    }

    StreamBuf& operator<<(unsigned char v) {
        return this->write("%u", v);
    }

    StreamBuf& operator<<(short v) {
        return this->write("%d", v);
    }

    StreamBuf& operator<<(unsigned short v) {
        return this->write("%u", v);
    }

    StreamBuf& operator<<(int v) {
        return this->write("%d", v);
    }

    StreamBuf& operator<<(unsigned int v) {
        return this->write("%u", v);
    }

    StreamBuf& operator<<(long v) {
        return this->write("%ld", v);
    }

    StreamBuf& operator<<(unsigned long v) {
        return this->write("%lu", v);
    }

    StreamBuf& operator<<(long long v) {
        return this->write("%lld", v);
    }

    StreamBuf& operator<<(unsigned long long v) {
        return this->write("%llu", v);
    }

    StreamBuf& operator<<(float v) {
        return this->write("%.7g", v);
    }

    StreamBuf& operator<<(double v) {
        return this->write("%.16lg", v);	//16
    }

    StreamBuf& operator<<(const char* v) {
        return this->append(v, ::strlen(v));
    }

    StreamBuf& operator<<(const std::string& v) {
        return this->append(v.data(), v.size());
    }

    StreamBuf& operator<<(const void* v) {
        return this->write("0x%llx", v);
    }

	inline bool header_has_read() const{
		return _rhead;
	}

	inline void mark_read_header(bool rhead){
		_rhead = rhead;
	}

	inline void set_body_len(int len){
		_bodylen = len;
	}

	inline int bodylen() const {
		return _bodylen;
	}

  private:
    template<typename T>
    StreamBuf& write(const char* fm, T t) {
        int x = static_cast<int>(_pend - _pcur);
        int r = ::snprintf(_pcur, x, fm, t);
        if (r < 0) return *this;

        if (r < x) {
            _pcur += r;
            return *this;
        }

        if (this->reserve(this->capacity() + r + 32)) {
            r = ::snprintf(_pcur, _pend - _pcur, fm, t);
            if (r >= 0) {
                _pcur += r;
                return *this;
            }
        }

        return *this;
    }

  private:
    char* _pbeg;		//begin
    char* _pcur;		//current
    char* _pend;		//end

	bool _rhead;	//header has been read
	int _bodylen;

    DISALLOW_COPY_AND_ASSIGN(StreamBuf);
};

//class safebuf {
//public:
//	safebuf() {}
//	~safebuf() {}
//
//	uint32 size() const {
//		allen::MutexGuard g(_mutex);
//		return _buf.size();
//	}
//
//	uint32 capacity() const {
//		allen::MutexGuard g(_mutex);
//		return _buf.capacity();		//return capacity
//	}
//
//	bool empty() const {
//		allen::MutexGuard g(_mutex);
//		return _buf.empty();
//	}
//
//	// not null-terminated
//	const char* data() const {
//		return _buf.data();
//	}
//
//	const char *data(uint32 n) const {
//		allen::MutexGuard g(_mutex);
//		return _buf.data();
//	}
//
//	const char *head() const {
//		return _buf.head();
//	}
//
//	const char *tail() const {
//		allen::MutexGuard g(_mutex);
//		return _buf.tail();
//	}
//
//	void addoffset(uint32 n) {
//		allen::MutexGuard g(_mutex);
//		_buf.addoffset(n);
//	}
//
//	std::string to_string() const {
//		return std::string(this->data(), this->size());
//	}
//
//	void clear() {
//		allen::MutexGuard g(_mutex);
//		_buf.clear();
//	}
//
//	void clear_head(uint32 n) {
//		allen::MutexGuard g(_mutex);
//		_buf.clear_head(n);
//	}
//
//	void clear_tail(uint32 n) {
//		allen::MutexGuard g(_mutex);
//		_buf.clear_tail(n);
//	}
//
//	void resize(uint32 n) {
//		allen::MutexGuard g(_mutex);
//		_buf.resize(n);
//	}
//
//	bool reserve(uint32 n) {
//		allen::MutexGuard g(_mutex);
//		return _buf.reserve(n);
//	}
//
//	safebuf& append(const void* data, uint32 size) {
//		allen::MutexGuard g(_mutex);
//		_buf.append(data, size);
//		return *this;
//	}
//
//	safebuf& operator<<(bool v) {
//		allen::MutexGuard g(_mutex);
//		return v ? this->append("true", 4) : this->append("false", 5);
//	}
//
//	safebuf& operator<<(char v) {
//		return this->append(&v, 1);
//	}
//
//	safebuf& operator<<(unsigned char v) {
//		return this->write("%u", v);
//	}
//
//	safebuf& operator<<(short v) {
//		return this->write("%d", v);
//	}
//
//	safebuf& operator<<(unsigned short v) {
//		return this->write("%u", v);
//	}
//
//	safebuf& operator<<(int v) {
//		return this->write("%d", v);
//	}
//
//	safebuf& operator<<(unsigned int v) {
//		return this->write("%u", v);
//	}
//
//	safebuf& operator<<(long v) {
//		return this->write("%ld", v);
//	}
//
//	safebuf& operator<<(unsigned long v) {
//		return this->write("%lu", v);
//	}
//
//	safebuf& operator<<(long long v) {
//		return this->write("%lld", v);
//	}
//
//	safebuf& operator<<(unsigned long long v) {
//		return this->write("%llu", v);
//	}
//
//	safebuf& operator<<(float v) {
//		return this->write("%.7g", v);
//	}
//
//	safebuf& operator<<(double v) {
//		return this->write("%.16lg", v);	//16
//	}
//
//	safebuf& operator<<(const char* v) {
//		return this->append(v, ::strlen(v));
//	}
//
//	safebuf& operator<<(const std::string& v) {
//		return this->append(v.data(), v.size());
//	}
//
//	safebuf& operator<<(const void* v) {
//		return this->write("0x%llx", v);
//	}
//
//private:
//	template<typename T>
//	safebuf& write(const char* fm, T t) {
//		_buf.write(fm, t);
//		return *this;
//	}
//
//private:
//	mutable allen::Mutex _mutex;
//	StreamBuf _buf;
//};