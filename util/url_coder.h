//#pragma once
#ifndef __URL_CODER_H_
#define __URL_CODER_H_

#include "../base/base.h"

/*
 * URL encoding & decoding implement
 *
 *   see https://en.wikipedia.org/wiki/Percent-encoding
 */

namespace util {

    std::string decode_url(const std::string &src);

    std::string encode_url(const std::string &src);

}

#endif