#include "../url_coder.h"

namespace {

class UrlCoder {
  public:
    UrlCoder();
	~UrlCoder() {};

    std::string decode(const std::string& src) const;
    std::string encode(const std::string& src) const;

    int hex2int(char c) const {
        if (c >= '0' && c <= '9') return c - '0';
        return ::tolower(c) - 'a' + 10;
    }

  private:

  private:
    ascii_bitset _uncoded;
    ascii_bitset _hex;

    DISALLOW_COPY_AND_ASSIGN(UrlCoder);
};

/*
 * reserved characters:  "!*'();:@&=+$,/?#[]"
 *
 * unreserved characters:  a-z A-Z 0-9 - _ . ~
 *
 * hex: 0-9 a-f A-F
 */

UrlCoder::UrlCoder()
    : _uncoded("-_.~!*'();:@&=+$,/?#[]") {

    for (int i = 'A'; i <= 'Z'; ++i) {
        _uncoded.set(i);
        _uncoded.set(i + 'a' - 'A');
    }

    for (int i = '0'; i <= '9'; ++i) {
        _uncoded.set(i);
        _hex.set(i);
    }

    for (int i = 'A'; i <= 'F'; ++i) {
        _hex.set(i);
        _hex.set(i + 'a' - 'A');
    }
}

std::string UrlCoder::decode(const std::string& src) const {
    std::string dst;
    dst.reserve(src.size());

    for (size_t i = 0; i < src.size(); ++i) {
        // for uncoded character
        if (_uncoded.has(src[i])) {
            dst.push_back(src[i]);
            continue;
        }

        // ' ' may be encoded as '+'
        if (src[i] == '+') {
            dst.push_back(' ');
            continue;
        }

        // for encoded character: %xx
        CHECK_EQ(src[i], '%');
        CHECK_LT(i + 2, src.size());
        CHECK(_hex.has(src[i + 1])) << static_cast<int>(src[i + 1]);
        CHECK(_hex.has(src[i + 2])) << static_cast<int>(src[i + 2]);

        int h4 = this->hex2int(src[i + 1]);
        int l4 = this->hex2int(src[i + 2]);

        char c = (h4 << 4) | l4;
        dst.push_back(c);

        i += 2;
    }

    return dst;
}

/*
 * todo: need encode reserved characters?
 */
std::string UrlCoder::encode(const std::string& src) const {
    std::string dst;

    for (size_t i = 0; i < src.size(); ++i) {
        if (_uncoded.has(src[i])) {
            dst.push_back(src[i]);
            continue;
        }

        dst.push_back('%');
        dst.push_back("0123456789ABCDEF"[static_cast<uint8>(src[i]) >> 4]);
        dst.push_back("0123456789ABCDEF"[static_cast<uint8>(src[i]) & 0x0F]);
    }

    return dst;
}

UrlCoder kUrlCoder;

} // namespace

namespace util {

std::string decode_url(const std::string& src) {
    return kUrlCoder.decode(src);
}

std::string encode_url(const std::string& src) {
    return kUrlCoder.encode(src);
}

} // namespace util


//long GetBig5Count(char *str)//如果此函数返回值大于零,则表明传入的字符串极可能是繁体
//{
//	int lnBIG5 = 0;//用於统计可能是繁体字的汉字个数
//	int lnGB = 0;//用於统计可能是简体字的汉字个数
//	int liTranLen = strlen(str);
//
//	for (int liT = 0; liT<liTranLen - 1; liT++)
//	{
//		//尾字节40-7E是BGI5码特有的,如果扫描到这种编码说明此字元串是繁体(经测试:有例外,可能是汉字的最後一个编码与英文编码组合而成的)
//		if ((BYTE)(BYTE)str[liT] >= 161 && (BYTE)(BYTE)str[liT] <= 254 && (BYTE)(BYTE)str[liT + 1] >= 64 && (BYTE)(BYTE)str[liT + 1] <= 126)
//			lnBIG5++;
//
//		//首字节A4-A9在GB中为日文假名,希腊字母,俄文字母和制表符,正常文本中很少出现,而这个范围是BIG5的常用汉字,所以认为这是BIG5码
//		if ((BYTE)(BYTE)str[liT] >= 164 && (BYTE)(BYTE)str[liT] <= 169 && (BYTE)(BYTE)str[liT + 1] >= 161 && (BYTE)(BYTE)str[liT + 1] <= 254)
//			lnBIG5++;
//
//		//GB中首字节AA-AF没有定义,所以首字节位於AA-AF之间,尾字节位於A1-FE的编码几乎100%是BIG5(经测试:没有100%),认为是BIG5码
//		if ((BYTE)(BYTE)str[liT] >= 170 && (BYTE)(BYTE)str[liT] <= 175 && (BYTE)(BYTE)str[liT + 1] >= 161 && (BYTE)(BYTE)str[liT + 1] <= 254)
//			lnBIG5++;
//
//		//首字节C6-D7,尾字节A1-FE在GB中属於一级字库,是常用汉字,而在BIG5中,C6-C7没有明确定义,但通常用来放日文假名和序号,C8-D7属於罕用汉字区,所以可认为是GB码
//		if ((BYTE)(BYTE)str[liT] >= 196 && (BYTE)(BYTE)str[liT] <= 215 && (BYTE)(BYTE)str[liT + 1] >= 161 && (BYTE)(BYTE)str[liT + 1] <= 254)
//			lnGB++;
//	}
//
//	//如果扫描完整个字元串,可能是简体字的数目比可能是繁体字的数目多就认为是简体字不转简(不一定准确)
//	return lnBIG5 - lnGB;
//}