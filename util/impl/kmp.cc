#include "../kmp.h"

namespace {

void compute_prefix(const std::string& substr, int next[]) {
    int j = -1;
    next[0] = j;

    for (int i = 1; i < substr.size(); ++i) {
        while (j > -1 && substr[j + 1] != substr[i]) {
            j = next[j];
        }

        if (substr[i] == substr[j + 1]) ++j;
        next[i] = j;
    }
}

size_t kmp_search(const char *text, size_t n, /* text length */
                  const char *substr, size_t m, /* substr length */
                  const int next[]) {
    int i;
    int j = -1;
    if (n == 0 && m == 0) return 0;
    if (m == 0) return 0;

    for (i = 0; i < n; i++) {
        while (j > -1 && substr[j + 1] != text[i]) {
            j = next[j];
        }

        if (text[i] == substr[j + 1]) ++j;
        if (j == m - 1) return i - j;
    }

    return (size_t) -1;
}

} // namespace

namespace util {

Kmp::Kmp(const std::string& substr)
    : _substr(substr) {
    _next.resize(_substr.size());
    compute_prefix(substr, &_next[0]);
}

size_t Kmp::search(const std::string& s, size_t from) const {
    size_t ret = kmp_search(s.data() + from, s.size() - from,
                            _substr.data(), _substr.size(), &_next[0]);
    return ret == (size_t) -1 ? ret : ret + from;
}


Kmp::~Kmp() {}

Kmp::Kmp(const Kmp&){}

void Kmp::operator=(const Kmp&){}

} // namespace util
