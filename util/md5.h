#pragma once

#include <string>

namespace util {

/*
 * default:
 *   return a 16-byte string may contain non-ASCII characters or null.
 *
 * if to_hex == true,
 *   return a string of length 32, containing only hexadecimal digits.
 */
std::string md5(const void* data, size_t len, bool to_hex = false);

inline std::string md5(const std::string& s, bool to_hex = false) {
    return md5(s.data(), s.size(), to_hex);
}

/*
 * md5 of file
 *
 *   return a string of length 32, containing only hexadecimal digits.
 */
std::string md5sum(const std::string& path);

} // namespace util
