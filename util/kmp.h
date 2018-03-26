#pragma once

#include <string>
#include <vector>

/*
http://www.cnblogs.com/c-cloud/p/3224788.html
*/

namespace util {

class Kmp {
  public:
    static const size_t npos = (size_t) -1;

    explicit Kmp(const std::string& substr);
	~Kmp();

   

    size_t search(const std::string& s, size_t from = 0) const;
  private:
	  Kmp(const Kmp&);
	  void operator=(const Kmp&);

  private:
    std::string _substr;
    std::vector<int> _next;
};

} // namespace util
