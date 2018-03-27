#pragma once

#include "data_types.h"
#include "stream_buf.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <bitset>
#include <algorithm>

namespace str {

/*
 *   *  matches everything
 *   ?  matches any single character
 */
bool fnmatch(const char* pattern, const char* expression,
             bool ignore_case = false);

inline bool fnmatch(const std::string& pattern, const std::string& expression,
             bool ignore_case = false) {
    return fnmatch(pattern.c_str(), expression.c_str(), ignore_case);
}

/*
 *   split("x||y", '|');  ==>  { "x", "", "y" }
 *   split("x  y", ' ');  ==>  { "x", "y" }
 */
std::vector<std::string> split(const std::string& s, char sep = ' ',
                               uint32 maxsplit = -1);

void split(const std::string& s, char sep,std::vector<std::string> &v,
		   uint32 maxsplit=-1);

std::vector<std::string> split(const std::string& s, const std::string& sep,
                               uint32 maxsplit = -1);

void split(const std::string& s, const std::string& sep,std::vector<std::string> &v,
		   uint32 maxsplit=-1);

/*
 *   replace("$@xx$@", "$@", "#");     return "#xx#"
 *   replace("$@xx$@", "$@", "#", 1);  return "#xx$@"
 */
std::string replace(const std::string& s, const std::string& sub,
                    const std::string& to, uint32 maxreplace = -1);

/*
 * replace characters in @chars to @c
 *
 *   replace("abc", "bc", 'x');  ==> "axx"
 */
std::string replace(const std::string& s, const std::string& chars,
                    char c, uint32 maxreplace = -1);

/*
 * if chars is empty, using " \t\n\x0b\x0c\r"
 *
 *   lstrip("$@xx@", "$@") ==> "xx@"
 *   rstrip("$@xx@", "$@") ==> "$@xx"
 *    strip("$@xx@", "$@") ==> "xx"
 */
std::string strip(const std::string& s, const std::string& chars = "");
void strip_nc(std::string& s, const std::string& chars = "");
void strip_nc(char *s, const std::string& chars = "");

std::string lstrip(const std::string& s, const std::string& chars = "");
void lstrip_nc(std::string& s, const std::string& chars = "");
void lstrip_nc(char *s, const std::string& chars = "");

std::string rstrip(const std::string& s, const std::string& chars = "");
void rstrip_nc(std::string& s, const std::string& chars = "");
void rstrip_nc(char *s, const std::string& chars = "");


/*
 * common_prefix({"xxa", "xxb", "xxc"}) ==> return "xx"
 */
std::string common_prefix(const std::vector<std::string>& v);

/*
 * upper("aBc") ==> return "ABC"
 * lower("aBc") ==> return "abc"
 * swapcase("aBc") ==> return "AbC"
 * capitalize("aBc") ==> return "ABc"
 */
std::string upper(const std::string& s);
void upper_nc(std::string& s);		//改变原对象

std::string lower(const std::string& s);
void lower_nc(std::string& s);		//改变原对象

std::string swapcase(const std::string& s);
void swapcase_nc(std::string& s);	//改变原对象

inline std::string capitalize(const std::string& s) {
    std::string v(s);
    if (!v.empty() && ::islower(*v.begin())) v[0] = ::toupper(v[0]);
    return v;
}

//返回每个汉字拼音首个字母：默认为返回小写字符串,最后一个参数传true返回大写字符串（eg. ）
std::string get_ab_pinyin(const std::string &strName,bool upper=false);
//返回所有汉字拼音字母：默认为返回小写字符串,最后一个参数传true返回大写字符串
void get_full_pinyin(unsigned char* Chinese, std::string& PinYin,bool upper=false);

/***  throw string exception on any error  ***/

// "true" or "1" ==> true,  "false" or "0" ==> false
bool to_bool(const std::string& v);

double to_double(const std::string& v);

int32 to_int32(const std::string& v);

int64 to_int64(const std::string& v);

uint32 to_uint32(const std::string& v);

uint64 to_uint64(const std::string& v);

/*
 *   vec = { "", "xx", "" };   to_string(vec, '|') ==> "||xx||"
 */
template <typename T>
std::string to_string(const std::vector<T>& v, char c = '|') {
    if (v.empty()) return std::string();

    StreamBuf sb;
    sb << c;

    for (uint32 i = 0; i < v.size(); ++i) {
        sb << v[i] << c;
    }

    return sb.to_string();
}

/*
 *   set<int> = { 3, 2, 1 };   to_string(set, '|') ==> "|1|2|3|"
 */
template <typename T>
std::string to_string(const std::set<T>& v, char c = '|') {
    if (v.empty()) return std::string();

    StreamBuf sb;
    sb << c;

    for (typename std::set<T>::iterator it = v.begin(); it != v.end(); ++it) {
        sb << *it << c;
    }

    return sb.to_string();
}

/*
 *  |k1:v1|k2:v2|k3:v3|
 *           |v1|v2|v3|     ignore_key = true
 */
template <typename K, typename V>
std::string to_string(const std::map<K, V>& v, bool ignore_key = false,
                      char c = '|') {
    if (v.empty()) return std::string();

    StreamBuf sb;
    sb << c;

    for (typename std::map<K, V>::iterator it = v.begin(); it != v.end(); ++it) {
        if (ignore_key) {
            sb << it->second << c;
        } else {
            sb << it->first << ':' << it->second << c;
        }
    }

    return sb.to_string();
}

template <typename A, typename B>
inline std::string to_string(const std::pair<A, B>& v) {
    StreamBuf sb;
    sb << "<" << v.first << ", " << v.second << ">";
    return sb.to_string();
}

/*
 * wchar to string, @wstr must be null-terminated
 */
std::string to_string(const wchar_t* wstr);

std::string to_string(const std::wstring& wstr);

/*
 * to_string(3.141);  ==> "3.141"
 * to_string(12345);  ==> "12345"
 */
template <typename T>
inline std::string to_string(T t) {
    StreamBuf sb(32);
    sb << t;
    return sb.to_string();
}

} // namespace str
