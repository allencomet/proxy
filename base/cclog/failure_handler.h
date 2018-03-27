#pragma once

#include <stdio.h>
//#include <functional>
#include <boost/function.hpp>

namespace cclog {
namespace xx {

//错误处理句柄（虚基类）
class FailureHandler {
  public:
	  FailureHandler() {};
	  virtual ~FailureHandler() {};

    virtual void set_fd(FILE* file) = 0;
    virtual void set_handler(boost::function<void()> cb) = 0;

  private:
	  FailureHandler(const FailureHandler &);
	  void operator=(const FailureHandler &);
};

FailureHandler* NewFailureHandler();

}  // namespace xx
}  // namespace cclog
