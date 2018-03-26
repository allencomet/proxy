#pragma once

#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef unsigned char   uchar;

#define MAX_UINT8 static_cast<uint8>(0xff)
#define MAX_UINT16 static_cast<uint16>(0xffff)
#define MAX_UINT32 static_cast<uint32>(0xffffffff)
#define MAX_UINT64 static_cast<uint64>(0xffffffffffffffff)

#define MIN_UINT8 static_cast<uint8>(0)
#define MIN_UINT16 static_cast<uint16>(0)
#define MIN_UINT32 static_cast<uint32>(0)
#define MIN_UINT64 static_cast<uint64>(0)

#define MAX_INT8 static_cast<int8>(0x7f)
#define MAX_INT16 static_cast<int16>(0x7fff)
#define MAX_INT32 static_cast<int32>(0x7fffffff)
#define MAX_INT64 static_cast<int64>(0x7fffffffffffffff)

#define MIN_INT8 static_cast<int8>(0x80)
#define MIN_INT16 static_cast<int16>(0x8000)
#define MIN_INT32 static_cast<int32>(0x80000000)
#define MIN_INT64 static_cast<int64>(0x8000000000000000)

template <typename T>
inline T abs(T t) {
    return t < 0 ? -t : t;
}

#define DISALLOW_COPY_AND_ASSIGN(Type) \
    Type(const Type&); \
    void operator=(const Type&)

template<typename To, typename From>
inline To cstyle_cast(From f) {
    return (To) f;
}


typedef	int32	SOCKET;
typedef	int32	BOOL;
#define    TRUE   1
#define    FALSE  0

#define    FAILURE              -1
#define    SUCCESS               0

#define    INVALID_SOCKET        -1
#define    SOCKET_ERROR          -1

typedef    struct sockaddr       SOCKADDR;
typedef    struct sockaddr_in    SOCKADDR_IN;

#ifndef FD_SETSIZE
#define FD_SETSIZE 256
#endif

#define MAXLINE 4096
#define LISTENQ 5
