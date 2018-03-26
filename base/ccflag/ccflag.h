#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace ccflag {
namespace std = ::std;

/*
 * parse command line flags
 *
 *   return non-flag elements
 *
 * usage:
 *   at the beginning of main(int argc, char** argv):
 *          ccflag::init_ccflag(argc, argv);
 *
 *   ./exe -boo --s=hello -i32=4k
 * or
 *   ./exe --config=xx.conf
 *
 *
 * config file xx.conf:
 *
 *   # # or //, comment
 *   # one flag per line, '\' for line continuation
 *   # ignore white spaces at the beginning or end of line
 *
 *     s = hello \
 *         world       // -s = "helloworld"
 *
 *     boo = true      // -boo = true
 *
 *   # value of the flag can be modified at runtime, if line begins with '!'
 *   ! i32 = 4k        // -i32 = 4096
 */
std::vector<std::string> init_ccflag(int argc, char** argv);

/*
 * set value of flag @name to @value, return error info if failed.
 *
 *   set_flag_value("xx", "true"); ==> --xx=true
 */
std::string set_flag_value(const std::string& name, const std::string& value);

namespace xx {
namespace std = ::std;
using std::string;

typedef ::int32_t int32;
typedef ::int64_t int64;
typedef ::uint32_t uint32;
typedef ::uint64_t uint64;

enum {
    TYPE_bool,
    TYPE_int32,
    TYPE_int64,
    TYPE_uint32,
    TYPE_uint64,
    TYPE_string,
    TYPE_double
};

struct FlagSaver {
    FlagSaver(const char* type_str, const char* name, const char* value,
              const char* help, const char* file, void* addr, int type);
};

}  // namespace xx
}  // namespace ccflag

#define DECLARE_CCFLAG(type, name) \
    namespace ccflag { \
    namespace zz { \
        using namespace ::ccflag::xx; \
        extern type FLG_##name; \
    } \
    } \
    using ccflag::zz::FLG_##name

/*
 * declare flag variable
 *
 *   DEC_string(s);  <==>  extern std::string FLG_s;
 *   DEC_bool(boo);  <==>  extern bool FLG_boo;
 */
#define DEC_bool(name)    DECLARE_CCFLAG(bool, name)
#define DEC_int32(name)   DECLARE_CCFLAG(int32, name)
#define DEC_int64(name)   DECLARE_CCFLAG(int64, name)
#define DEC_uint32(name)  DECLARE_CCFLAG(uint32, name)
#define DEC_uint64(name)  DECLARE_CCFLAG(uint64, name)
#define DEC_string(name)  DECLARE_CCFLAG(string, name)
#define DEC_double(name)  DECLARE_CCFLAG(double, name)

#define DEFINE_CCFLAG(type, name, value, help) \
    namespace ccflag { \
    namespace zz { \
        using namespace ::ccflag::xx; \
        type FLG_##name = value; \
        static ::ccflag::xx::FlagSaver kFs_##name( \
            #type, #name, #value, help, __FILE__, &FLG_##name, \
            ::ccflag::xx::TYPE_##type \
        ); \
    } \
    } \
    using ccflag::zz::FLG_##name

/*
 * define flag variable
 *
 *   DEF_int32(i, 23, "help info here");  ==>  int32 FLG_i = 23
 */
#define DEF_bool(name, value, help)    DEFINE_CCFLAG(bool, name, value, help)
#define DEF_int32(name, value, help)   DEFINE_CCFLAG(int32, name, value, help)
#define DEF_int64(name, value, help)   DEFINE_CCFLAG(int64, name, value, help)
#define DEF_uint32(name, value, help)  DEFINE_CCFLAG(uint32, name, value, help)
#define DEF_uint64(name, value, help)  DEFINE_CCFLAG(uint64, name, value, help)
#define DEF_string(name, value, help)  DEFINE_CCFLAG(string, name, value, help)
#define DEF_double(name, value, help)  DEFINE_CCFLAG(double, name, value, help)

DEC_string(config);
