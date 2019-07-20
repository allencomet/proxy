#ifndef __BASE_H_
#define __BASE_H_

#include "ccflag/ccflag.h"
#include "cclog/cclog.h"
#include "cctest/cctest.h"


#include "ascii.h"
#include "data_types.h"
#include "stream_buf.h"

#include "os.h"
#include "sys.h"
#include "net_util.h"
#include "string_util.h"
#include "thread_util.h"


#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/shm.h>
#include <sys/sem.h>


#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <deque>
#include <bitset>
#include <memory>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <unordered_set>


#ifdef __GNUC__

#include <ext/hash_map>

namespace __gnu_cxx {
    template<>
    struct hash<std::string> {
        size_t operator()(const std::string &x) const {
            return hash<const char *>()(x.c_str());
        }
    };

    template<>
    struct hash<long long> {
        size_t operator()(long long x) const {
            return x;
        }
    };
}
#else
#include <hash_map>
#endif


namespace std {
    using namespace std::placeholders;
}


#define    MIN(a, b)    ((a) < (b) ? (a) : (b))
#define    MAX(a, b)    ((a) > (b) ? (a) : (b))

#endif