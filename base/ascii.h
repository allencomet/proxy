#pragma once

#include "data_types.h"

#include <algorithm>
#include <bitset>
#include <map>
#include <vector>

/*
 * usage:
 *   ascii_bitset ab("xyz");
 *
 *   ab.has('x');    // return true
 *   ab.hasno('z');  // return false
 *   ab.has('o');    // return false
 */
class ascii_bitset {
  public:
	  ascii_bitset() {};
	  ~ascii_bitset() {};

    explicit ascii_bitset(const std::string& s) {
      #if __cpp_lambdas >= 200907
        std::for_each(s.begin(), s.end(), [this](char c) { this->set(c); });
      #else
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] >= 0) this->set(s[i]);
        }
      #endif
    }

    void set(char c) {
        if (c >= 0) _bits[c] = 1;		//c用作下标时会转换成ascii的值
    }

    void unset(char c) {
        if (c >= 0) _bits[c] = 0;
    }

    bool has(char c) const {
        return c >= 0 ? _bits[c] != 0 : false;
    }

    bool hasno(char c) const {
        return !this->has(c);
    }

  private:
	ascii_bitset(const ascii_bitset &) {}
	void operator=(const ascii_bitset&){}
  private:
    std::bitset<128> _bits;

    //DISALLOW_COPY_AND_ASSIGN(ascii_bitset);
};

/*
 * usage:
 *   ascii_table tab {
 *       std::map<char, int> { {'x', 1}, {'y', 2}, {'z', 3} }
 *   };
 *
 *   tab.has('x');  // return true;
 *   tab.set('z', 7);
 *   tab.get('z');  // return 7
 */
class ascii_table {
  public:
    ascii_table() {
        init();
    }

    ascii_table(const std::map<char, int>& x){
		init();
        for (std::map<char, int>::const_iterator it = x.begin(); it != x.end(); ++it) {
            if (it->first >= 0) _table[it->first] = it->second;
        }
    }

	~ascii_table() {};

	void init(){
		_table.resize(128);
	}

    void set(char c, int value) {
        if (c >= 0) _table[c] = value;
    }

    void unset(char c) {
        if (c >= 0) _table[c] = 0;
    }

    int get(char c) const {
        return c >= 0 ? _table[c] : 0;
    }

    bool has(char c) const {
        return c >= 0 ? _table[c] != 0 : false;
    }

    bool hasno(char c) const {
        return !this->has(c);
    }

  private:
	ascii_table(const ascii_table &){}
	void operator=(const ascii_table&){}

  private:
    std::vector<int> _table;

    //DISALLOW_COPY_AND_ASSIGN(ascii_table);
};
