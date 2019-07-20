#include "../url_coder.h"

namespace {

    class UrlCoder {
    public:
        UrlCoder();

        ~UrlCoder() {};

        std::string decode(const std::string &src) const;

        std::string encode(const std::string &src) const;

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

    std::string UrlCoder::decode(const std::string &src) const {
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
    std::string UrlCoder::encode(const std::string &src) const {
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

    std::string decode_url(const std::string &src) {
        return kUrlCoder.decode(src);
    }

    std::string encode_url(const std::string &src) {
        return kUrlCoder.encode(src);
    }

} // namespace util


//long GetBig5Count(char *str)//����˺�������ֵ������,�����������ַ����������Ƿ���
//{
//	int lnBIG5 = 0;//���ͳ�ƿ����Ƿ����ֵĺ��ָ���
//	int lnGB = 0;//���ͳ�ƿ����Ǽ����ֵĺ��ָ���
//	int liTranLen = strlen(str);
//
//	for (int liT = 0; liT<liTranLen - 1; liT++)
//	{
//		//β�ֽ�40-7E��BGI5�����е�,���ɨ�赽���ֱ���˵������Ԫ���Ƿ���(������:������,�����Ǻ��ֵ�����һ��������Ӣ�ı�����϶��ɵ�)
//		if ((BYTE)(BYTE)str[liT] >= 161 && (BYTE)(BYTE)str[liT] <= 254 && (BYTE)(BYTE)str[liT + 1] >= 64 && (BYTE)(BYTE)str[liT + 1] <= 126)
//			lnBIG5++;
//
//		//���ֽ�A4-A9��GB��Ϊ���ļ���,ϣ����ĸ,������ĸ���Ʊ��,�����ı��к��ٳ���,�������Χ��BIG5�ĳ��ú���,������Ϊ����BIG5��
//		if ((BYTE)(BYTE)str[liT] >= 164 && (BYTE)(BYTE)str[liT] <= 169 && (BYTE)(BYTE)str[liT + 1] >= 161 && (BYTE)(BYTE)str[liT + 1] <= 254)
//			lnBIG5++;
//
//		//GB�����ֽ�AA-AFû�ж���,�������ֽ�λ�AA-AF֮��,β�ֽ�λ�A1-FE�ı��뼸��100%��BIG5(������:û��100%),��Ϊ��BIG5��
//		if ((BYTE)(BYTE)str[liT] >= 170 && (BYTE)(BYTE)str[liT] <= 175 && (BYTE)(BYTE)str[liT + 1] >= 161 && (BYTE)(BYTE)str[liT + 1] <= 254)
//			lnBIG5++;
//
//		//���ֽ�C6-D7,β�ֽ�A1-FE��GB�����һ���ֿ�,�ǳ��ú���,����BIG5��,C6-C7û����ȷ����,��ͨ�����������ļ��������,C8-D7��춺��ú�����,���Կ���Ϊ��GB��
//		if ((BYTE)(BYTE)str[liT] >= 196 && (BYTE)(BYTE)str[liT] <= 215 && (BYTE)(BYTE)str[liT + 1] >= 161 && (BYTE)(BYTE)str[liT + 1] <= 254)
//			lnGB++;
//	}
//
//	//���ɨ����������Ԫ��,�����Ǽ����ֵ���Ŀ�ȿ����Ƿ����ֵ���Ŀ�����Ϊ�Ǽ����ֲ�ת��(��һ��׼ȷ)
//	return lnBIG5 - lnGB;
//}