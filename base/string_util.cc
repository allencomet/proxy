#include "string_util.h"
#include "ascii.h"
#include "cclog/cclog.h"

#include <wchar.h>
#include <iconv.h>
#include <errno.h>
#include <math.h>

namespace str {

bool fnmatch(const char* p, const char* e, bool ignore_case) {
    if (*p == '*' && *(p + 1) == '\0') return true;

    for (; *p != '\0' && *e != '\0';) {
        char c = *p;

        if (c == '*') {
            return fnmatch(p + 1, e, ignore_case) ||
                   fnmatch(p, e + 1, ignore_case);
        }

        if (c == '?' || c == *e || (ignore_case && tolower(c) == tolower(*e))) {
            ++p;
            ++e;
            continue;
        }

        return false;
    }

    return (*p == '*' && *(p + 1) == '\0') || (*p == '\0' && *e == '\0');
}

std::vector<std::string> split(const std::string& s, char sep,
                               uint32 maxsplit) {
    std::vector<std::string> v;
    bool white_separ = (sep == ' ' || sep == '\t');

    ::size_t pos = 0, from = 0;
    if (!s.empty() && sep != '\n' && s[0] == sep) from = 1;

    while ((pos = s.find(sep, from)) != std::string::npos) {
        if (!white_separ || pos != from) {
            v.push_back(s.substr(from, pos - from));
            if (v.size() == maxsplit) return v;
        }
        from = pos + 1;
    }

    if (from < s.size()) v.push_back(s.substr(from));

    return v;
}

void split(const std::string& s, char sep,std::vector<std::string> &v,
		   uint32 maxsplit) {
			   v.clear();
			   bool white_separ = (sep == ' ' || sep == '\t');

			   ::size_t pos = 0, from = 0;
			   if (!s.empty() && sep != '\n' && s[0] == sep) from = 1;

			   while ((pos = s.find(sep, from)) != std::string::npos) {
				   if (!white_separ || pos != from) {
					   v.push_back(s.substr(from, pos - from));
					   if (v.size() == maxsplit) return;
				   }
				   from = pos + 1;
			   }

			   if (from < s.size()) v.push_back(s.substr(from));
}

std::vector<std::string> split(const std::string& s, const std::string& sep,
                               uint32 maxsplit) {
    std::vector<std::string> v;

    ::size_t pos = 0, from = 0;

    while ((pos = s.find(sep, from)) != std::string::npos) {
        if (pos != 0) v.push_back(s.substr(from, pos - from));
        if (v.size() == maxsplit) return v;
        from = pos + sep.size();
    }

    if (from < s.size()) v.push_back(s.substr(from));

    return v;
}

void split(const std::string& s, const std::string& sep,std::vector<std::string> &v,
		   uint32 maxsplit) {
	v.clear();

	::size_t pos = 0, from = 0;

	while ((pos = s.find(sep, from)) != std::string::npos) {
		if (pos != 0) v.push_back(s.substr(from, pos - from));
		if (v.size() == maxsplit) return;
		from = pos + sep.size();
	}

	if (from < s.size()) v.push_back(s.substr(from));	
}

std::string replace(const std::string& s, const std::string& sub,
                    const std::string& to, uint32 maxreplace) {
    std::string x;
    ::size_t pos = 0, from = 0;
    uint32 i = 0;

    while ((pos = s.find(sub, from)) != s.npos) {
        x += s.substr(from, pos - from) + to;
        from = pos + sub.size();
        if (++i == maxreplace) break;
    }

    if (from == 0) return s;

    if (from < s.size()) x += s.substr(from);
    return x;
}

std::string replace(const std::string& s, const std::string& chars,
                    char c, uint32 maxreplace) {
    if (s.empty() || chars.empty()) return s;

    ascii_bitset ab(chars);

    std::string v(s);
    uint32 num = 0;

    for (size_t i = 0; i < v.size(); ++i) {
        if (ab.has(v[i])) {
            v[i] = c;
            if (++num == maxreplace) break;
        }
    }

    return v;
}

std::string strip(const std::string& s, const std::string& chars) {
    if (s.empty()) return s;

    ascii_bitset ab(!chars.empty() ? chars : " \t\n\x0b\x0c\r");
    if (!ab.has(s[0]) && !ab.has(*s.rbegin())) return s;

    ::size_t bp = 0, ep = s.size();
    for (; bp < s.size(); ++bp) {
        if (!ab.has(s[bp])) break;		//find the first not of specify word and break
    }

    if (bp == s.size()) return std::string();	//if doesn't find and return an empty sting

    for (; ep > 0; --ep) {
        if (!ab.has(s[ep - 1])) break;	//find the last not of specify word and break
    }

    return s.substr(bp, ep - bp);	
}

void strip_nc(std::string& s, const std::string& chars){
	if (s.empty()) return;

	ascii_bitset ab(!chars.empty() ? chars : " \t\n\x0b\x0c\r");
	if (!ab.has(s[0]) && !ab.has(*s.rbegin())) return;		//if head and tail none and return 

	::size_t bp = 0, ep = s.size();
	for (; bp < s.size(); ++bp) {
		if (!ab.has(s[bp])) break;		//find the first not of specify word and break
	}

	if (bp == s.size()) return;	//if doesn't find and return an empty sting

	for (; ep > 0; --ep) {
		if (!ab.has(s[ep - 1])) break;	//find the last not of specify word and break
	}

	s = s.substr(bp, ep - bp);	
}

void strip_nc(char *s, const std::string& chars){
	lstrip_nc(s,chars);
	rstrip_nc(s,chars);
}

std::string lstrip(const std::string& s, const std::string& chars) {
    if (s.empty()) return s;

    ascii_bitset ab(!chars.empty() ? chars : " \t\n\x0b\x0c\r");
    if (!ab.has(s[0])) return s;

    ::size_t bp = 1;
    for (; bp < s.size(); ++bp) {
        if (!ab.has(s[bp])) break;
    }

    if (bp == s.size()) return std::string();
    return s.substr(bp);
}

void lstrip_nc(std::string& s, const std::string& chars){
	if (s.empty()) return;

	ascii_bitset ab(!chars.empty() ? chars : " \t\n\x0b\x0c\r");
	if (!ab.has(s[0])) return;

	::size_t bp = 1;
	for (; bp < s.size(); ++bp) {
		if (!ab.has(s[bp])) break;
	}

	if (bp == s.size()) return;
	s = s.substr(bp);
}

void lstrip_nc(char *s, const std::string& chars){
	if (NULL == s) return;

	ascii_bitset ab(!chars.empty() ? chars : " \t\n\x0b\x0c\r");
	if (!ab.has(s[0])) return;

	char *p = NULL;
	for (p = s; ; ++p) {
		if (!ab.has(*p)) break;
	}

	memmove(s,p,strlen(s)-(p-s));
	s[strlen(s)-(p-s)] = '\0';
}

std::string rstrip(const std::string& s, const std::string& chars) {
    if (s.empty()) return s;

    ascii_bitset ab(!chars.empty() ? chars : " \t\n\x0b\x0c\r");
    if (!ab.has(*s.rbegin())) return s;

    ::size_t ep = s.size() - 1;

    for (; ep > 0; --ep) {
        if (!ab.has(s[ep - 1])) break;
    }

    return s.substr(0, ep);
}

void rstrip_nc(std::string& s, const std::string& chars){
	if (s.empty()) return;

	ascii_bitset ab(!chars.empty() ? chars : " \t\n\x0b\x0c\r");
	if (!ab.has(*s.rbegin())) return;

	::size_t ep = s.size() - 1;

	for (; ep > 0; --ep) {
		if (!ab.has(s[ep - 1])) break;
	}

	s = s.substr(0, ep);
}

void rstrip_nc(char *s, const std::string& chars){
	if (NULL == s || s[0] == '\0') return;

	ascii_bitset ab(!chars.empty() ? chars : " \t\n\x0b\x0c\r");

	char *ptr = NULL;

	for (char *ptr = s + strlen( s ) - 1; ptr > s ; --ptr) {
		if (!ab.has(*ptr))
		{
			*(++ptr) = 0x0;		break;
		}
	}
}

std::string common_prefix(const std::vector<std::string>& v) {
    if (v.empty()) return std::string();
    if (v.size() == 1) return v[0];

    uint32 x = 0;
    for (uint32 i = 0; i < v[0].size(); ++i) {
        for (uint32 j = 1; j < v.size(); ++j) {
            if (i >= v[j].size() || v[0][i] != v[j][i]) return v[0].substr(0, x);
        }

        ++x;
    }

    return v[0].substr(0, x);
}

std::string upper(const std::string& s) {
    std::string v(s);

    for (size_t i = 0; i < v.size(); ++i) {
        if (::islower(v[i])) v[i] = ::toupper(v[i]);
    }

    return v;
}

void upper_nc(std::string& s){
	for (size_t i = 0; i < s.size(); ++i) {
		if (::islower(s[i])) s[i] = ::toupper(s[i]);
	}

	return;
}

std::string lower(const std::string& s) {
    std::string v(s);

    for (size_t i = 0; i < v.size(); ++i) {
        if (::isupper(v[i])) v[i] = ::tolower(v[i]);
    }

    return v;
}

void lower_nc(std::string& s){
	for (size_t i = 0; i < s.size(); ++i) {
		if (::isupper(s[i])) s[i] = ::tolower(s[i]);
	}

	return;
}

std::string swapcase(const std::string& s) {
    std::string v(s);

    for (size_t i = 0; i < v.size(); ++i) {
        char c = v[i];
        if (::islower(c)) {
            v[i] = ::toupper(c);
        } else if (::isupper(c)) {
            v[i] = ::tolower(c);
        }
    }

    return v;
}

void swapcase_nc(std::string& s){
	for (size_t i = 0; i < s.size(); ++i) {
		char c = s[i];
		if (::islower(c)) {
			s[i] = ::toupper(c);
		} else if (::isupper(c)) {
			s[i] = ::tolower(c);
		}
	}

	return;
}

static void str_remove_char(std::string &strValue, char cMove){
	std::string::iterator itr;
	for (itr = strValue.begin(); itr != strValue.end(); ++itr)
	{
		if ( *itr == cMove)	strValue.erase(itr);
	}
}

static char get_cn_pinyin(unsigned long wChinese)
{
	union SWord
	{
		unsigned long wWord;
		struct
		{
			unsigned char	cLow;
			unsigned char	cHigh;
		}c;
	}code;

	code.wWord = wChinese;
	unsigned char c = code.c.cHigh;
	code.c.cHigh = code.c.cLow;
	code.c.cLow = c;

	// 汉字的高字节小於0x80，即为英文字母或系统char
	if (code.c.cHigh < 0x80)
		return 0;

	static int mapC[] = {0x0b0a1, 0x0b0c5, 0x0b2c1, 0x0b4ee, 0x0b6ea, 0x0b7a2, 0x0b8c1, 0x0b9fe,
		0x0bbf7, 0x0bfa6, 0x0c0ac, 0x0c2e8, 0x0c4c3, 0x0c5b6, 0x0c5be, 0x0c6da,
		0x0c8bb, 0x0c8f6, 0x0cbfa, 0x0cdda, 0x0cef4, 0x0d1b9, 0x0d4d1, 0x0d8a1};

	static int mapE[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'W', 'X', 'Y', 'Z'};

	static char mapCex[] = "cjwgnspgcgne?y?btyyzdxykygt?jnmjqmbsgzscyjsyy?pgkbzgy?ywjkgkljswkpjqhy?"
		"w?dzlsgmrypywwcckznkyygttnjjeykkzytcjnmcylqlypyqfqrpzslwbtgkjfyxjwzltbncxjjjjtxdttsqzycdxx"
		"hgck?phffss?ybgmxlpbyll?hlxs?zm?jhsojnghdzqyklgjh?gqzhxqgkezzwyscscjxyeyxadzpmdjsmzjzqjyzc?"
		"j?wqjbdzpxgznzcpwhkxhqkmwfbpbydtjzzkqxylygxfptyjyyzpszlfchmqshgmxxsxj??dcsbbqbefsjyhxwgzkp"
		"ylqbgldlcdtnmaydtkssngycsgxlyzaypnptsdkdylhgymydcqpy?jndqjwxqxfyyfjlejbzrxccqwqqsbzkymgplb"
		"mjrqcflnymyqmtqtybcjthztqfrxqhxmjjcjlyxgjmshzkbswyemyltxfsydsglycjqxsjnqbsctyhbftdcyzdjwyg"
		"hqfrxwckqkxebptlpxjzsrmebwhjlbjslyysmdxlclqkxlhxjrzjmfqhxhwywsbhtrxxglhqhfnm?ikldyxzpylgg?"
		"mtcfpajjzyljtyanjgbjplqgdzyqyaxbkysecjsznslyzhzxlzcghpxzhznytdsbcjkdlzayfmydlebdgqyzkxgldn"
		"dnyskjshdlyxbzgyxypkdjmmzngmmclgnzszxzjfznmlzzthcsydbdllscddnlkjykjsycjlkwhqasdknhcsganhda"
		"ashtcplcpqybsdmpjlpzjoqlcdhjjysprchr?jnlhlyyqyhwzptczgwwmzffjqqqqyxaclbhkdjxdgmmydjxzllsyg"
		"xgkjrywzwyclzmssjzldbydcfcxyhlxchyzjq??qagmnyxpfrkssbjlyxysyglnscmhcwwmnzjjlxxhchsy??xtxry"
		"cyxbyhcsmxjsznpwgpxxtaybgajcxly?dccwzocwkccsbnhcpdyznfcyytyckxkybsqkkytqqxftwchcykelzqbsqy"
		"jqcclmthsywhmktlkjlycxwhyqqhtqh?pq?qscfymmdmgbwhwlgsllysdlmlxpthmjhwljzyhzjxktxjlhxrswlwzj"
		"cbxmhzqxsdzpmgfcsglsxymjshxpjxwmyqksmyplrthbxftpmhyxlchlhlzylxgsssstclsldclrpbhzhxyy?hb?gd"
		"mycnqqwlqhjj?ywjzyejjdhpblqxtqkwhdchqxagtlxljxmsl?htzkzjecxjcjnmfby?sfywybjzgnysdzsqyrsljp"
		"clpwxsdwejbjcbcnaytwgmpapclyqpclzxsbnmsggfnzjjbzsf?yntxhplqkzczwalsbccjx?yzgwkypsgxfzfcdk?"
		"jgxtlqfsgdslqwzkxtmhsbgzmjzrglyjbpmlmsxlzjqqhz?j?zydjwb?jklddpmjegxyhylxhlqyqhkycwcjmyyxna"
		"tjhyccxzpcqlbzwwytwsqcmlpmyrjcccxfpznzzljplxxyztzlgdldcklyrzzgqtgjhhgjljaxfgfjzslcfdqzlclg"
		"jdjcsnclljpjqdcclcjxmyzftsxgcgsbrzxjqqctzhgyqtjqqlzxjylylbcyamcstylpdjbyregklzyzhlyszqlznw"
		"czcll?jjjjjkdgjzolbbzp?glghtgzxyghzmycnqsycyhbhgxknmtxyxnjskyzzzjzlqjdfcjxdygjqjjpmgwgjjjp"
		"kqsbgbmmcjssclpqpbxcdyyky?cjddyygywbhjrtgznyqldkljszzgqzjgdykshpzmtlcpwnjafyzdjcnmwescyglb"
		"tzcgmssllyxqsxs?sjsbb?gghfjlypmzjnlyywdqshzxtyywhmzyhywdbxbtlmsyyyfsxj??dxxlhjhf?sazqhfzmz"
		"cztqcxzxrttdjhnnyzqqmtqdmmg?ydxmjgdhcdyzbffallztdltfxmxqzdngwqdbdczjdxbzgsqqddjcmbkzffxmkd"
		"mdsyyszcmljdsynsbrskmkmpcklgdbqtfzswtfgglyplljzhgj?gypzltcsmcnbtjbqfhdhbyzgkpbpymtdssxtbnp"
		"dkleycjnycdykztdhqhsdzsctarlltkzlgec?lkjlqjaqnbdkkg?pjtzqksecshalqfmmgjnlyjbbtmlyzxdtjpldl"
		"pcqdhzycbzsczbzmsljflkrzjsnfrgjhxpdhyjybzgdlqcsezg?lblgyxtwmabchecmwyjyzlljjyhlg?djlslygkd"
		"zpzxjyyzlwcxszfgwyydlyhcljscmbjhblyclylblydpdqysxqzbqtdkyxjy?cnrjmpdjgklcljbzxbjddbblblczq"
		"rppxjcjlzcshltoljnmdddlngkaqhqhjgykheznmshrp?qqjchgmfprxhjgdychgklyrzqlcyzjnzsqtkqjymszxwl"
		"cfqqqxyfggyptqwlmcrnfkkfsyylqbmqammmyxctpshcptxxzzsmphpshmclmldqfyqxszyjdjjzzhqpdszglstjbc"
		"kbxyqzjsgpsxqzqzrqtbdkyxzkhhgflbcsmdldgdzdblzyycxn?csybzbfglzzxswmsccmqnjqsbdqsjtxxmbltxzc"
		"lzshzcxrqjgjylxzfjphy?zqqydfqjjlzznzjgdgz?gctxlzylctlkphtxhtlbjxjlxscdqxcbbtjdqzfsltjbtkqb"
		"xxjjljchczdbzjdczjdcprnpqcjpfjzlclzxzdmxmphjsgzgszzqlylwtjpfsyasmcjbtzkycwmytzsjjlzc?lwzma"
		"lbxyfbpnlsfhtgjwejjxxglljstgshjqlzfkcgnndszfdeqfhbsaqtgylbxmmygszldydq?jjrgbjtkgdhgkblqkbd"
		"mbylxwcxyttybkmrtjzxqjbhlmhmjjzmqasldcyxyqdlqcafywyxqhz";

	// 超出内码表范围
	if (code.wWord >= 0x0d8a1)
	{
		unsigned short w = 0x5e * (code.c.cHigh - 0xd8) + code.c.cLow - 0xa1;
		if (w < 0 || w > sizeof(mapCex))
			return '?';
		else
			return mapCex[w];
	}

	for(int i=0; i<23; i++)
	{
		if( code.wWord >= mapC[22-i] )
			return mapE[22-i];
	}

	if(code.c.cHigh == 0x0A3)
	{
		if (code.c.cLow >= 0xb0 && code.c.cLow <= 0xb9)
			return code.c.cLow - 0xb0 + '0';
		if (code.c.cLow >= 0xc1 && code.c.cLow <= 0xda)
			return code.c.cLow - 0xc1 + 'a';
		if (code.c.cLow >= 0xe1 && code.c.cLow <= 0xfa)
			return code.c.cLow - 0xe1 + 'a';
	}
	return '?';
}

std::string get_ab_pinyin(const std::string &name,bool upper)
{
	std::string py = "";

	unsigned char cRet;
	char tmp[2];
	for (int i = 0; i < name.size()&& i<20; )		//最长只能是20个字符
	{
		cRet = (unsigned char)name[i];
		if (cRet >= 0x80)
		{
			tmp[0] = name[i]; tmp[1] = name[i+1];
			cRet = ::toupper(get_cn_pinyin(*((unsigned short*)tmp)));
			if (cRet != 0)
				py += cRet;
			i++; i++;
		}
		else
		{
			if ((cRet >= '0' && cRet <= '9') || (cRet >= 'A' && cRet <= 'Z')
				|| (cRet >='a' && cRet <='z'))
				py += cRet;
			i++;
		}
	}

	// 去掉 "?" 字符
	str_remove_char(py, '?');
	str_remove_char(py, 0x20);
	std::string strNum = "                  ";
	int iIndex = name.find(strNum);
	if (iIndex != std::string::npos)
	{
		py += name.substr(iIndex, 2);
	}

	/*for (int i = 0 ;i <m_nPinYinNumbers && i<MAX_PINYIN_NUM;i++)
	{
		if (strcmp(strName,m_strPinYin[i][0])== 0)
		{
			strPinYin = m_strPinYin[i][1];
			break;
		}
	}*/

	if (!upper)	lower_nc(py);

	return py;
}

std::string pinyin[] = 
{
	"a",    "ai",    "an",    "ang",    "ao",    "ba",    "bai",    "ban",    "bang",    "bao",
	"bei",    "ben",    "beng",    "bi",    "bian",    "biao",    "bie",    "bin",    "bing",    "bo",
	"bu",    "ca",    "cai",    "can",    "cang",    "cao",    "ce",    "ceng",    "cha",    "chai",
	"chan",    "chang","chao",    "che",    "chen",    "cheng","chi",    "chong","chou",    "chu",
	"chuai","chuan","chuang","chui","chun",    "chuo",    "ci",    "cong",    "cou",    "cu",
	"cuan",    "cui",    "cun",    "cuo",    "da",    "dai",    "dan",    "dang",    "dao",    "de",
	"deng",    "di",    "dian",    "diao",    "die",    "ding",    "diu",    "dong",    "dou",    "du",
	"duan",    "dui",    "dun",    "duo",    "e",    "en",    "er",    "fa",    "fan",    "fang",
	"fei",    "fen",    "feng",    "fo",    "fou",    "fu",    "ga",    "gai",    "gan",    "gang",
	"gao",    "ge",    "gei",    "gen",    "geng",    "gong",    "gou",    "gu",    "gua",    "guai",
	"guan",    "guang","gui",    "gun",    "guo",    "ha",    "hai",    "han",    "hang",    "hao",
	"he",    "hei",    "hen",    "heng",    "hong",    "hou",    "hu",    "hua",    "huai",    "huan",
	"huang","hui",    "hun",    "huo",    "ji",    "jia",    "jian",    "jiang","jiao",    "jie",    
	"jin",    "jing",    "jiong","jiu",    "ju",    "juan",    "jue",    "jun",    "ka",    "kai",
	"kan",    "kang",    "kao",    "ke",    "ken",    "keng",    "kong",    "kou",    "ku",    "kua",
	"kuai",    "kuan",    "kuang","kui",    "kun",    "kuo",    "la",    "lai",    "lan",    "lang",
	"lao",    "le",    "lei",    "leng",    "li",    "lia",    "lian",    "liang","liao",    "lie",
	"lin",    "ling",    "liu",    "long",    "lou",    "lu",    "lv",    "luan",    "lue",    "lun",
	"luo",    "ma",    "mai",    "man",    "mang",    "mao",    "me",    "mei",    "men",    "meng",
	"mi",    "mian",    "miao",    "mie",    "min",    "ming",    "miu",    "mo",    "mou",    "mu",    
	"na",    "nai",    "nan",    "nang",    "nao",    "ne",    "nei",    "nen",    "neng",    "ni",
	"nian",    "niang","niao",    "nie",    "nin",    "ning",    "niu",    "nong",    "nu",    "nv",
	"nuan",    "nue",    "nuo",    "o",    "ou",    "pa",    "pai",    "pan",    "pang",    "pao",
	"pei",    "pen",    "peng",    "pi",    "pian",    "piao",    "pie",    "pin",    "ping",    "po",
	"pou",    "pu",    "qi",    "qia",    "qian",    "qiang","qiao",    "qie",    "qin",    "qing",
	"qiong","qiu",    "qu",    "quan",    "que",    "qun",    "ran",    "rang",    "rao",    "re",
	"ren",    "reng",    "ri",    "rong",    "rou",    "ru",    "ruan",    "rui",    "run",    "ruo",    
	"sa",    "sai",    "san",    "sang",    "sao",    "se",    "sen",    "seng",    "sha",    "shai",
	"shan",    "shang","shao",    "she",    "shen",    "sheng","shi",    "shou",    "shu",    "shua",    
	"shuai","shuan","shuang","shui","shun",    "shuo",    "si",    "song",    "sou",    "su",
	"suan",    "sui",    "sun",    "suo",    "ta",    "tai",    "tan",    "tang",    "tao",    "te",
	"teng",    "ti",    "tian",    "tiao",    "tie",    "ting",    "tong",    "tou",    "tu",    "tuan",
	"tui",    "tun",    "tuo",    "wa",    "wai",    "wan",    "wang",    "wei",    "wen",    "weng",    
	"wo",    "wu",    "xi",    "xia",    "xian",    "xiang","xiao",    "xie",    "xin",    "xing",    
	"xiong","xiu",    "xu",    "xuan",    "xue",    "xun",    "ya",    "yan",    "yang",    "yao",
	"ye",    "yi",    "yin",    "ying",    "yo",    "yong",    "you",    "yu",    "yuan",    "yue",
	"yun",    "za",    "zai",    "zan",    "zang",    "zao",    "ze",    "zei",    "zen",    "zeng",
	"zha",    "zhai",    "zhan",    "zhang","zhao",    "zhe",    "zhen",    "zheng","zhi",    "zhong",
	"zhou",    "zhu",    "zhua",    "zhuai","zhuan","zhuang","zhui","zhun",    "zhuo",    "zi",
	"zong",    "zou",    "zu",    "zuan",    "zui",    "zun",    "zuo"
};

int code_qw[] = 
{
	1601,1603,1616,1625,1628,1637,1655,1663,1678,1690,
	1713,1728,1732,1738,1762,1774,1778,1782,1788,1803,
	1822,1833,1834,1845,1852,1857,1862,1867,1869,1880,
	1883,1893,1912,1921,1927,1937,1952,1968,1973,1985,
	2007,2008,2015,2021,2026,2033,2035,2047,2053,2054,
	2058,2061,2069,2072,2078,2084,2102,2117,2122,2134,
	2137,2144,2163,2179,2188,2201,2210,2211,2221,2228,
	2243,2249,2253,2262,2274,2287,2288,2302,2310,2327,
	2338,2350,2365,2380,2381,2382,2433,2435,2441,2452,
	2461,2471,2488,2489,2491,2504,2519,2528,2546,2552,
	2555,2566,2569,2585,2588,2594,2601,2608,2627,2630,
	2639,2657,2659,2663,2668,2677,2684,2708,2717,2722,
	2736,2750,2771,2777,2787,2846,2863,2909,2922,2950,
	2977,3003,3028,3030,3047,3072,3079,3089,3106,3110,
	3115,3121,3128,3132,3147,3151,3153,3157,3161,3168,
	3173,3177,3179,3187,3204,3208,3212,3219,3222,3237,
	3244,3253,3255,3266,3269,3309,3310,3324,3335,3348,
	3353,3364,3379,3390,3405,3411,3431,3445,3451,3453,
	3460,3472,3481,3487,3502,3508,3520,3521,3537,3540,
	3548,3562,3571,3579,3581,3587,3593,3594,3617,3620,
	3635,3642,3647,3650,3651,3656,3657,3659,3660,3661,
	3672,3679,3681,3683,3690,3691,3703,3707,3711,3714,
	3715,3716,3718,3722,3723,3730,3736,3742,3750,3755,
	3762,3771,3773,3787,3810,3814,3818,3820,3825,3834,
	3842,3843,3858,3894,3903,3925,3933,3948,3953,3964,
	3977,3979,3987,4006,4017,4025,4027,4031,4036,4039,
	4041,4051,4053,4054,4064,4067,4077,4079,4082,4084,
	4086,4089,4093,4103,4106,4110,4113,4114,4115,4124,
	4126,4142,4150,4161,4173,4189,4206,4253,4263,4302,
	4304,4308,4310,4313,4317,4321,4325,4341,4349,4353,
	4365,4368,4379,4382,4390,4405,4414,4432,4445,4456,
	4457,4461,4476,4484,4489,4492,4508,4521,4525,4536,
	4538,4544,4547,4558,4565,4567,4584,4594,4633,4643,
	4646,4655,4684,4725,4738,4764,4784,4809,4829,4839,
	4854,4861,4870,4889,4905,4911,4925,4941,4974,4991,
	5012,5027,5080,5102,5120,5121,5136,5156,5207,5227,
	5237,5249,5252,5259,5263,5266,5280,5284,5285,5286,
	5290,5310,5316,5333,5348,5358,5368,5384,5405,5448,
	5459,5473,5505,5507,5508,5514,5521,5527,5529,5540,
	5555,5562,5566,5574,5576,5580,5582
};

static void get_other_code_py(int nCode, std::string& strValue)
{
	switch(nCode) 
	{ 
	case 6325: case 6436: case 7571: case 7925:  strValue="a";  break; 
	case 6263: case 6440: case 7040: case 7208: case 7451: case 7733: case 7945: case 8616:  strValue="ai";  break;
	case 5847: case 5991: case 6278: case 6577: case 6654: case 7281: case 7907: case 8038: case 8786:  strValue="an";  break; 
		strValue="ang";  break;
	case 5974: case 6254: case 6427: case 6514: case 6658: case 6959: case 7033: case 7081: case 7365: case 8190: case 8292: case 8643: case 8701: case 8773:  strValue="ao";  break;
	case 6056: case 6135: case 6517: case 7857: case 8446: case 8649: case 8741:  strValue="ba";  break;
	case 6267: case 6334: case 7494:  strValue="bai";  break;
	case 5870: case 5964: case 7851: case 8103: case 8113: case 8418:  strValue="ban";  break;
	case 6182: case 6826:  strValue="bang";  break;
	case 6165: case 7063: case 7650: case 8017: case 8157: case 8532: case 8621:  strValue="bao";  break;
	case 5635: case 5873: case 5893: case 5993: case 6141: case 6703: case 7753: case 8039: case 8156: case 8645: case 8725:  strValue="bei";  break;
	case 5946: case 5948: case 7458: case 7928:  strValue="ben";  break;
	case 6452: case 7420:  strValue="beng";  break;
	case 5616: case 5734: case 6074: case 6109: case 6221: case 6333: case 6357: case 6589: case 6656: case 6725: case 6868: case 6908: case 6986: case 6994: case 7030: case 7052: case 7221: case 7815: case 7873: case 7985: case 8152: case 8357: case 8375: case 8387: case 8416: case 8437: case 8547: case 8734:  strValue="bi";  break;
	case 5650: case 5945: case 6048: case 6677: case 6774: case 7134: case 7614: case 7652: case 7730: case 7760: case 8125: case 8159: case 8289: case 8354: case 8693:  strValue="bian";  break;
	case 7027: case 7084: case 7609: case 7613: case 7958: case 7980: case 8106: case 8149: case 8707: case 8752:  strValue="biao";  break;
	case 8531:  strValue="bie";  break;
	case 5747: case 6557: case 7145: case 7167: case 7336: case 7375: case 7587: case 7957: case 8738: case 8762:  strValue="bin";  break;
	case 5787: case 5891: case 6280:  strValue="bing";  break;
	case 5781: case 6403: case 6636: case 7362: case 7502: case 7771: case 7864: case 8030: case 8404: case 8543: case 8559:  strValue="bo";  break;
	case 6318: case 6945: case 7419: case 7446: case 7848: case 7863: case 8519:  strValue="bu";  break;
	case 6474: case 7769:  strValue="ca";  break;
		strValue="cai";  break;
	case 6978: case 7078: case 7218: case 8451: case 8785:  strValue="can";  break;
	case 5687:  strValue="cang";  break;
	case 6448: case 6878: case 8309: case 8429:  strValue="cao";  break;
	case 6692:  strValue="ce";  break;
	case 6515: case 6825:  strValue="cen";  break;
	case 6465:  strValue="ceng";  break;
	case 6639: case 6766: case 7017: case 7230: case 7311: case 7322: case 7363: case 7942: case 7979: case 8135:  strValue="cha";  break;
	case 5713: case 7846: case 8091: case 8218:  strValue="chai";  break;
	case 5770: case 5838: case 6159: case 6667: case 6893: case 6904: case 6981: case 7031: case 7086: case 7472: case 7688: case 7966: case 8324: case 8580:  strValue="chan";  break;
	case 5686: case 5943: case 6041: case 6137: case 6660: case 6568: case 6749: case 7029: case 7047: case 7438: case 7509: case 8680:  strValue="chang";  break;
	case 6687: case 7443: case 8173:  strValue="chao";  break;
	case 5969: case 7726:  strValue="che";  break;
	case 5840: case 5863: case 6251: case 6433: case 6923: case 7201: case 7320: case 7755: case 8619:  strValue="chen";  break;
	case 5609: case 5984: case 7239: case 7263: case 7583: case 7810: case 7881: case 7905: case 8146: case 8241: case 8508:  strValue="cheng";  break;
	case 5749: case 6015: case 6061: case 6319: case 6374: case 6420: case 6445: case 6633: case 7042: case 7523: case 7787: case 8023: case 8101: case 8161: case 8231: case 8304: case 8355: case 8388: case 8489: case 8556: case 8746:  strValue="chi";  break;
	case 6091: case 6671: case 6731: case 8409: case 8430:  strValue="chong";  break;
	case 5717: case 6492: case 6716: case 8112: case 8637:  strValue="chou";  break;
	case 5601: case 5927: case 6680: case 6732: case 7109: case 7238: case 7290: case 7343: case 8150: case 8260: case 8573: case 8777:  strValue="chu";  break;
	case 6285: case 6408: case 7590: case 8563:  strValue="chuai";  break;
	case 6622: case 6955: case 7516: case 7843: case 8413:  strValue="chuan";  break;
	case 6675:  strValue="chuang";  break;
	case 5879: case 7302: case 7319:  strValue="chui";  break;
	case 6127: case 8040: case 8277:  strValue="chun";  break;
	case 7401: case 8554: case 8626:  strValue="chuo";  break; strValue="ci";  break;
	case 6075: case 6358: case 7684: case 8043: case 8457:  strValue="ci";  break;
	case 6042: case 6840: case 7085: case 7193: case 7214: case 7240:  strValue="cong";  break;
	case 7308: case 7403: case 7577:  strValue="cou";  break;
	case 6180: case 6562: case 6607: case 7367: case 8501: case 8530: case 8577:  strValue="cu";  break;
	case 5764: case 6305: case 7664: case 7973:  strValue="cuan";  break;
	case 6718: case 6145: case 6393: case 7213: case 7333: case 7505: case 8631:  strValue="cui";  break;
	case 6666: case 8169:  strValue="cun";  break;
	case 5640: case 6547: case 7566: case 7917: case 7983: case 8078: case 8526: case 8567:  strValue="cuo";  break;
	case 6239: case 6353: case 6410: case 6682: case 7007: case 8155: case 8346: case 8716: case 8718:  strValue="da";  break;
	case 6004: case 6316: case 6523: case 6942: case 7110: case 7173: case 8776:  strValue="dai";  break;
	case 5757: case 6144: case 6402: case 7373: case 7470: case 7781: case 8067: case 8087: case 8185: case 8376:  strValue="dan";  break;
	case 5852: case 5942: case 6148: case 6920: case 7724: case 7885: case 8141:  strValue="dang";  break;
	case 6322: case 6665: case 7514: case 8478:  strValue="dao";  break;
	case 7929:  strValue="de";  break;
	case 6466: case 6556: case 7413: case 7767: case 7975: case 8403:  strValue="deng";  break;
	case 5621: case 5765: case 5814: case 5848: case 5901: case 5970: case 6122: case 6454: case 7023: case 7116: case 7260: case 7306: case 7475: case 7738: case 7758: case 7791: case 7965: case 8438: case 8730:  strValue="di";  break;
	case 6439:  strValue="dia";  break;
	case 5871: case 5967: case 6559: case 7172: case 7868: case 8116: case 8118: case 8401: case 8558:  strValue="dian";  break;
	case 7886: case 8585: case 8684:  strValue="diao";  break;
	case 5976: case 6006: case 6273: case 6409: case 7526: case 8012: case 8183: case 8562: case 8688:  strValue="die";  break;
	case 5674: case 6404: case 7164: case 7575: case 7754: case 7814: case 8059: case 8184: case 8490:  strValue="ding";  break;
	case 7891:  strValue="diu";  break;
	case 5977: case 6343: case 6520: case 6528: case 7517: case 7543: case 7556: case 7747: case 8020:  strValue="dong";  break;
	case 6190: case 8128: case 8229: case 8391:  strValue="dou";  break;
	case 6022: case 6429: case 6834: case 7292: case 7525: case 8328: case 8338: case 8739: case 8782:  strValue="du";  break;
	case 7318: case 7649: case 8393:  strValue="duan";  break;
	case 7701: case 7713: case 7752:  strValue="dui";  break;
	case 6771: case 7632: case 7727: case 7766: case 7779: case 7970: case 8527:  strValue="dun";  break;
	case 6345: case 6365: case 6785: case 7122: case 7876: case 8154: case 8566:  strValue="duo";  break;
	case 5612: case 5832: case 5844: case 5949: case 6035: case 6113: case 6164: case 6332: case 6721: case 6977: case 7025: case 7378: case 7581: case 7916: case 7941: case 8042: case 8206: case 8689:  strValue="e";  break;
	case 6176: case 6284:  strValue="en";  break;
	case 5706: case 6939: case 7177: case 7879: case 8025: case 8660:  strValue="er";  break;
	case 5950: case 7732:  strValue="fa";  break;
	case 6212: case 6232: case 6506: case 7283: case 7660: case 7818: case 8576:  strValue="fan";  break;
	case 5890: case 7242: case 7853: case 8419: case 8648:  strValue="fang";  break;
	case 6032: case 6584: case 6713: case 6839: case 6990: case 7119: case 7328: case 7572: case 7619: case 7673: case 7948: case 8082: case 8267: case 8385: case 8468: case 8613: case 8678:  strValue="fei";  break;
	case 5739: case 6915: case 7291: case 8687: case 8787:  strValue="fen";  break;
	case 5726: case 5926: case 6155: case 6384: case 6767: case 7731:  strValue="feng";  break;
		strValue="fo";  break;
	case 8330:  strValue="fou";  break;
	case 5775: case 5776: case 5914: case 6029: case 6062: case 6119: case 6142: case 6252: case 6327: case 6505: case 6686: case 6870: case 6985: case 7058: case 7066: case 7106: case 7108: case 7285: case 7471: case 7680: case 7741: case 7774: case 7775: case 7823: case 7991: case 8005: case 8222: case 8261: case 8280: case 8283: case 8479: case 8535: case 8538: case 8654: case 8691:  strValue="fu";  break;
	case 6246: case 7056: case 7057: case 7424: case 7837:  strValue=" ga";  break;
	case 5604: case 5875: case 5982: case 7414: case 7464:  strValue="gai";  break;
	case 5965: case 6053: case 6247: case 6306: case 6779: case 6838: case 6887: case 7104: case 7347: case 7426: case 7723: case 8065: case 8491:  strValue="gan";  break;
	case 7716: case 7824: case 8364:  strValue="gang";  break;
	case 5626: case 5830: case 5912: case 6227: case 7141: case 7332: case 7334: case 7429: case 7915:  strValue="gao";  break;
	case 5610: case 5678: case 5933: case 5957: case 6010: case 6435: case 7092: case 7501: case 7585: case 7749: case 7951: case 8143: case 8220: case 8420: case 8732:  strValue="ge";  break;
		strValue="gei";  break;
	case 5608: case 6102: case 6371: case 8462:  strValue="gen";  break;
	case 6376: case 6657: case 7114: case 8665:  strValue="geng";  break;
	case 7178: case 7537: case 8228: case 8601:  strValue="gong";  break;
	case 5694: case 5824: case 6524: case 6960: case 7037: case 7135: case 7259: case 7477: case 7616: case 8349: case 8384: case 8724:  strValue="gou";  break;
	case 5637: case 5812: case 6152: case 6536: case 6773: case 7284: case 7379: case 7484: case 7486: case 7591: case 7617: case 7813: case 7825: case 7860: case 7932: case 8019: case 8083: case 8233: case 8494: case 8593: case 8681: case 8729:  strValue="gu";  break;
	case 5652: case 5820: case 6341: case 7273: case 7550: case 8027:  strValue="gua";  break;
		strValue="guai";  break;
	case 5736: case 6124: case 6272: case 6842: case 7834: case 8057: case 8170: case 8704:  strValue="guan";  break;
	case 6359: case 6578: case 7270: case 7555:  strValue="guang";  break;
	case 5648: case 5659: case 6649: case 7003: case 7277: case 7433: case 7448: case 8007: case 8394: case 8657: case 8712:  strValue="gui";  break;
	case 5782: case 7121: case 7762: case 8671:  strValue="gun";  break;
	case 5769: case 6266: case 6335: case 6494: case 6538: case 6603: case 7304: case 7529: case 8188: case 8268: case 8269:  strValue="guo";  break;
	case 7894:  strValue="ha";  break;
	case 6443: case 7560: case 8516:  strValue="hai";  break;
	case 5885: case 6153: case 6294: case 6759: case 6911: case 7447: case 7642: case 8192: case 8205: case 8232: case 8793:  strValue="han";  break;
	case 6776: case 7112: case 8194:  strValue="hang";  break;
	case 6179: case 6222: case 6438: case 6467: case 6909: case 6916: case 7427: case 8009: case 8211: case 8226:  strValue="hao";  break;
	case 5813: case 5932: case 5954: case 6432: case 6756: case 7434: case 7833: case 8202: case 8234: case 8471:  strValue="he";  break;
		strValue="hei";  break;
		strValue="hen";  break;
	case 6231: case 7181: case 7276:  strValue="heng";  break;
	case 5768: case 5774: case 5807: case 6106: case 6214: case 6216: case 6740: case 6792:  strValue="hong";  break;
	case 6009: case 6565: case 6943: case 8090: case 8383: case 8455: case 8655: case 8731:  strValue="hou";  break;
	case 5792: case 6392: case 6481: case 6518: case 6609: case 6679: case 6717: case 6816: case 6879: case 7190: case 7346: case 7385: case 7618: case 7635: case 7646: case 7670: case 7672: case 7679: case 8013: case 8032: case 8041: case 8055: case 8343: case 8513: case 8590:  strValue="hu";  break;
	case 7072: case 7275: case 7725: case 7892:  strValue="hua";  break;
	case 8555:  strValue="huai";  break;
	case 5928: case 6140: case 6307: case 6487: case 6621: case 6801: case 6829: case 6881: case 6930: case 6953: case 7157: case 7944: case 8673: case 8763:  strValue="huan";  break;
	case 5882: case 6569: case 6850: case 6874: case 6956: case 7211: case 7533: case 8105: case 8308: case 8382: case 8692:  strValue="huang";  break;
	case 5822: case 6078: case 6086: case 6205: case 6352: case 6360: case 6425: case 6736: case 6807: case 6811: case 6971: case 7132: case 7185: case 7445: case 7703: case 8219: case 8319: case 8766:  strValue="hui";  break;
	case 5827: case 6638: case 6752: case 6867:  strValue="hun";  break;
	case 5669: case 6229: case 6311: case 6475: case 6623: case 7856: case 7933: case 7976: case 8175: case 8322:  strValue="huo";  break;
	case 5629: case 5632: case 5662: case 5705: case 5742: case 5952: case 6024: case 6033: case 6193: case 6210: case 6265: case 6320: case 6350: case 6383: case 6507: case 6553: case 6809: case 6976: case 7087: case 7160: case 7165: case 7314: case 7374: case 7410: case 7411: case 7469: case 7473: case 7487: case 7620: case 7722: case 7831: case 7990: case 8002: case 8104: case 8217: case 8337: case 8339: case 8463: case 8550: case 8611: case 8661: case 8674: case 8757: case 8768:  strValue="ji";  break;
	case 5704: case 5903: case 6171: case 6521: case 6804: case 6940: case 7176: case 7409: case 7546: case 7702: case 7882: case 7956: case 8072: case 8142: case 8244: case 8353: case 8434: case 8542:  strValue="jia";  break;
	case 5752: case 5841: case 5857: case 6149: case 6183: case 6286: case 6853: case 6931: case 6932: case 7144: case 7237: case 7305: case 7407: case 7415: case 7480: case 7489: case 7506: case 7576: case 7790: case 7921: case 8047: case 8148: case 8340: case 8469: case 8534: case 8561: case 8668: case 8721:  strValue="jian";  break;
	case 6092: case 6814: case 7113: case 7154: case 7481: case 7768: case 8180: case 8461: case 8488:  strValue="jiang";  break;
	case 5714: case 5753: case 6020: case 6090: case 6256: case 6461: case 6572: case 7015: case 7524: case 8008: case 8052: case 8252: case 8520: case 8551: case 8662:  strValue="jiao";  break;
	case 5806: case 5821: case 6255: case 6414: case 7028: case 7061: case 7278: case 7757: case 8060: case 8201: case 8227: case 8441: case 8658: case 8726:  strValue="jie";  break;
	case 5865: case 6103: case 6132: case 6468: case 6643: case 6659: case 7138: case 7210: case 7340: case 7465: case 7478: case 8138:  strValue="jin";  break;
	case 5751: case 5869: case 6128: case 6616: case 6729: case 6794: case 6941: case 6982: case 7026: case 7534: case 7554: case 7570: case 7626:  strValue="jiang";  break;
	case 6936: case 7671:  strValue="jiong";  break;
	case 5754: case 6417: case 6746: case 7249: case 7274: case 8015: case 8053: case 8481: case 8761:  strValue="jiu";  break;
	case 5738: case 5810: case 6036: case 6058: case 6076: case 6268: case 6965: case 6980: case 7202: case 7307: case 7316: case 7323: case 7357: case 7381: case 7488: case 7611: case 7850: case 7924: case 8022: case 8132: case 8153: case 8482: case 8522: case 8565: case 8620: case 8634: case 8722:  strValue="ju";  break;
	case 5918: case 6590: case 6824: case 7280: case 7835: case 7935: case 7952: case 8633:  strValue="juan";  break;
	case 5642: case 5667: case 5860: case 5939: case 6207: case 6421: case 6457: case 6469: case 6540: case 6617: case 7062: case 7169: case 7286: case 7351: case 7663: case 7967: case 8574: case 8591:  strValue="jue";  break;
	case 6260: case 8168: case 8362: case 8769:  strValue="jun";  break;
	case 5671: case 6339: case 7544:  strValue="ka";  break;
	case 5660: case 5978: case 6160: case 6673: case 6693: case 7888: case 7920: case 7939:  strValue="kai";  break;
	case 5709: case 6108: case 7412: case 7772: case 7811:  strValue="kan";  break;
	case 5688: case 6742: case 7854:  strValue="kang";  break;
	case 6974: case 7264: case 7491: case 7877:  strValue="kao";  break;
	case 6430: case 6519: case 6701: case 6859: case 7076: case 7128: case 7170: case 7380: case 7520: case 7807: case 7861: case 7930: case 7993: case 8066: case 8129: case 8204: case 8282: case 8733:  strValue="ke";  break;
	case 8144:  strValue="ken";  break;
	case 7912:  strValue="keng";  break;
	case 5737: case 6539: case 8377:  strValue="kong";  break;
	case 6050: case 6202: case 6321: case 7778: case 8356:  strValue="kou";  break;
	case 5658: case 6005: case 6423: case 7111: case 8728:  strValue="ku";  break;
	case 5708:  strValue="kua";  break;
	case 5665: case 5906: case 6364: case 6586: case 7558:  strValue="kuai";  break;
	case 8737:  strValue="kuan";  break;
	case 5818: case 5831: case 5887: case 5959: case 6237: case 6349: case 7094: case 7460:  strValue="kuang";  break;
	case 5624: case 5649: case 5771: case 6162: case 6281: case 6413: case 6416: case 6720: case 6951: case 7450: case 7805: case 8606: case 8743:  strValue="kui";  break;
	case 6204: case 6245: case 6458: case 6618: case 6928: case 7152: case 7841: case 8051:  strValue="liao";  break;
	case 5793: case 5988: case 6270: case 6354: case 6803: case 8483: case 8581: case 8764:  strValue="lie";  break;
	case 6194: case 6388: case 6555: case 6662: case 6733: case 6964: case 7361: case 7405: case 7602: case 7812: case 8452: case 8579: case 8775:  strValue="lin";  break;
	case 5925: case 6063: case 6342: case 6482: case 6786: case 7117: case 7258: case 7289: case 7418: case 8186: case 8240: case 8465: case 8676:  strValue="ling";  break;
	case 6815: case 6962: case 7082: case 7124: case 7628: case 7654: case 7919: case 7954: case 8050: case 8644:  strValue="liu";  break;
	case 5966: case 6055: case 6781: case 7171: case 7248: case 7542: case 7735: case 8110:  strValue="long";  break;
	case 5745: case 6168: case 6422: case 6548: case 7946: case 8092: case 8179: case 8287: case 8735:  strValue="lou";  break;
	case 6744: case 7321: case 7586: case 7918: case 7989: case 8158:  strValue="lv";  break;
	case 5968: case 6303: case 6464: case 6782: case 6843: case 6885: case 6954: case 7220: case 7251: case 7354: case 7391: case 7404: case 7510: case 7545: case 7969: case 8021: case 8056: case 8392: case 8421: case 8652:  strValue="lu";  break;
	case 5785: case 7014: case 7279: case 8029: case 8639:  strValue="luan";  break;
		strValue="lve";  break;
		strValue="lun";  break;
	case 5732: case 5789: case 6093: case 6259: case 6291: case 6604: case 6788: case 6880: case 7183: case 7301: case 7565: case 7961: case 8107: case 8635:  strValue="luo";  break;
	case 6328:  strValue="m";  break;
	case 6373: case 6579: case 7054: case 7231: case 8301:  strValue="ma";  break;
	case 5929: case 6104: case 8618:  strValue="mai";  break;
	case 6012: case 6503: case 7147: case 7655: case 7960: case 8209: case 8293: case 8709: case 8720:  strValue="man";  break;
	case 5888: case 6861: case 7743: case 8294:  strValue="mang";  break;
	case 5783: case 6066: case 6525: case 6787: case 7203: case 7436: case 7483: case 7503: case 7624: case 7714: case 7806: case 8317: case 8754:  strValue="mao";  break;
	case 6114: case 6550: case 6613: case 6828: case 6856: case 7325: case 7949: case 8044: case 8139: case 8740:  strValue="mei";  break;
	case 6249: case 7643: case 7715: case 7845:  strValue="men";  break;
	case 5934: case 6189: case 6211: case 6734: case 7592: case 7770: case 8221: case 8276: case 8323: case 8427: case 8431:  strValue="meng";  break;
	case 5634: case 5855: case 6234: case 6368: case 6455: case 6608: case 6772: case 6921: case 6984: case 7563: case 7682: case 8445: case 8767: case 8771:  strValue="mi";  break;
	case 6770: case 6837: case 6847: case 7579: case 7777:  strValue="mian";  break;
	case 6387: case 6967: case 7131: case 7149: case 7234: case 7721: case 7780: case 8037:  strValue="miao";  break;
	case 5631: case 6367: case 8326: case 8390:  strValue="mie";  break;
	case 6069: case 6526: case 6741: case 6793: case 7137: case 7168: case 7175: case 7710: case 8710: case 8628:  strValue="min";  break;
	case 5804: case 6088: case 6873: case 7452: case 7808: case 8504:  strValue="ming";  break;
		strValue="miu";  break;
	case 5851: case 6052: case 6175: case 6641: case 7038: case 7366: case 7950: case 7987: case 8102: case 8182: case 8586: case 8588: case 8765:  strValue="mo";  break;
	case 5716: case 6372: case 7788: case 8254: case 8290: case 8642:  strValue="mou";  break;
	case 5679: case 5973: case 6057: case 6769: case 7504: case 7866:  strValue="mu";  break;
	case 6437:  strValue="n";  break;
	case 6264: case 7539: case 7953: case 8136:  strValue="na";  break;
	case 5630: case 6021: case 6133: case 7245:  strValue="nai";  break;
	case 6411: case 6478: case 6479: case 7310: case 7578: case 8279: case 8486:  strValue="nan";  break;
	case 6313: case 6476: case 6646: case 7457:  strValue="nang";  break;
	case 5611: case 5981: case 6346: case 6614: case 7207: case 7748: case 7883: case 8245:  strValue="nao";  break;
	case 5811:  strValue="ne";  break;
		strValue="nei";  break;
	case 7705:  strValue="nen";  break;
		strValue="neng";  break;
	case 5703: case 5972: case 6605: case 6685: case 7439: case 7627: case 7711: case 7794: case 7874: case 8682:  strValue="ni";  break;
	case 5605: case 5994: case 7393: case 8004: case 8651: case 8683:  strValue="nian";  break;
		strValue="niang";  break;
	case 6064: case 7053: case 7569: case 8433:  strValue="niao";  break;
	case 5877: case 6233: case 6431: case 8208: case 8411: case 8570:  strValue="nie";  break;
		strValue="nin";  break;
	case 5690: case 6344: case 6924: case 8187:  strValue="ning";  break;
	case 6580: case 6678: case 7004:  strValue="niu";  break;
	case 5715: case 6370:  strValue="nong";  break;
	case 8181:  strValue="nou";  break;
	case 6983: case 7032: case 7059: case 7069:  strValue="nu";  break;
	case 7704: case 7847: case 8412:  strValue="nv";  break;
		strValue="nuan";  break;
		strValue="nue";  break;
	case 5748: case 6289: case 6386: case 7927:  strValue="nuo";  break;
	case 6424: case 6462:  strValue="o";  break;
	case 5809: case 6670: case 7417: case 8178:  strValue="ou";  break;
	case 6166: case 7243: case 8365:  strValue="pa";  break;
	case 5729: case 6169: case 6363:  strValue="pai";  break;
	case 6761: case 6790: case 8140: case 8165: case 8320: case 8571:  strValue="pan";  break;
	case 6561: case 6872: case 6944: case 8306:  strValue="pang";  break;
	case 6243: case 6583: case 6650: case 7567: case 8069:  strValue="pao";  break;
	case 6446: case 6490: case 7623: case 7934: case 8512: case 8612:  strValue="pei";  break;
	case 6852:  strValue="pen";  break;
	case 6001: case 6456: case 6681: case 8318:  strValue="peng";  break;
	case 5607: case 5682: case 5880: case 5892: case 5915: case 5960: case 6017: case 6037: case 6308: case 6472: case 6647: case 6836: case 7039: case 7102: case 7233: case 7422: case 7802: case 7828: case 7875: case 8117: case 8166: case 8223: case 8271: case 8589:  strValue="pi";  break;
	case 5850: case 7073: case 7490: case 7561: case 8470: case 8568:  strValue="pian";  break;
	case 5666: case 6449: case 7046: case 7146: case 7372: case 7809: case 8310:  strValue="piao";  break;
	case 6054: case 7513:  strValue="pie";  break;
	case 7041: case 6253: case 7016: case 7315: case 7482: case 8213:  strValue="pin";  break;
	case 5723: case 7019: case 7250: case 8650:  strValue="ping";  break;
	case 5647: case 5922: case 7174: case 7839: case 7862: case 8011: case 8345:  strValue="po";  break;
	case 5786: case 6269:  strValue="pou";  break;
	case 5773: case 6459: case 6863: case 6907: case 7217: case 7511: case 7968: case 7972: case 8575:  strValue="pu";  break;
	case 5633: case 5725: case 5963: case 6027: case 6046: case 6089: case 6129: case 6134: case 6161: case 6213: case 6366: case 6450: case 6508: case 6510: case 6764: case 6831: case 7075: case 7118: case 7187: case 7189: case 7229: case 7271: case 7342: case 7440: case 7605: case 7687: case 7712: case 7751: case 8193: case 8251: case 8264: case 8475: case 8476: case 8572: case 8702: case 8772:  strValue="qi";  break;
	case 6154: case 8736:  strValue="qia";  break;
	case 5727: case 5761: case 5868: case 6023: case 6045: case 6071: case 6271: case 6509: case 6705: case 6727: case 6925: case 6926: case 6929: case 7155: case 7293: case 7541: case 7709: case 7852: case 8215: case 8373:  strValue="qian";  break;
	case 6762: case 7045: case 7341: case 7408: case 7633: case 7926: case 7947: case 7974: case 8163: case 8262: case 8439: case 8536:  strValue="qiang";  break;
	case 5668: case 5829: case 5859: case 6081: case 6529: case 6724: case 6730: case 7352: case 7745: case 8546: case 8719:  strValue="qiao";  break;
	case 5907: case 6711: case 7010: case 7492: case 7938: case 8370:  strValue="qie";  break;
	case 6043: case 6276: case 6336: case 6426: case 6463: case 6858: case 7353: case 7923: case 8291: case 8432:  strValue="qin";  break;
	case 6060: case 6485: case 7349: case 7764: case 8263: case 8332: case 8368: case 8605: case 8675: case 8784:  strValue="qing";  break;
	case 5886: case 6068: case 8123: case 8243: case 8344: case 8528: case 8638:  strValue="qiong";  break;
	case 5720: case 5947: case 6576: case 6848: case 6947: case 6957: case 7317: case 7468: case 8216: case 8239: case 8288: case 8435: case 8460: case 8690: case 8792:  strValue="qiu";  break;
	case 5816: case 5930: case 6201: case 6230: case 6511: case 6573: case 6754: case 7219: case 7479: case 7512: case 7552: case 7678: case 7765: case 8119: case 8248: case 8329: case 8480: case 8636: case 8781:  strValue="qu";  break;
	case 5825: case 6085: case 6710: case 7125: case 7390: case 7816: case 7893: case 8273: case 8360: case 8760:  strValue="quan";  break;
	case 6755: case 6758: case 7708:  strValue="que";  break;
	case 6950:  strValue="qun";  break;
	case 6059: case 8237: case 8755:  strValue="ran";  break;
	case 7692: case 8006:  strValue="rang";  break;
	case 6073: case 7012: case 7267:  strValue="rao";  break;
		strValue="re";  break;
	case 5680: case 6083: case 6156: case 6631: case 7377: case 7994: case 8137:  strValue="ren";  break;
		strValue="reng";  break;
		strValue="ri";  break;
	case 6541: case 6585: case 7337: case 7532: case 8278:  strValue="rong";  break;
	case 8459: case 8569: case 8723:  strValue="rou";  break;
	case 6174: case 6224: case 6473: case 6818: case 6865: case 6906: case 7140: case 7908: case 8164: case 8212:  strValue="ru";  break;
	case 7535:  strValue="ruan";  break;
	case 6039: case 6208: case 7236: case 7803: case 8224:  strValue="rui";  break;
		strValue="run";  break;
	case 5728: case 8372:  strValue="ruo";  break;
	case 5606: case 5677: case 7493: case 7559: case 7610:  strValue="sa";  break;
	case 6471:  strValue="sai";  break;
	case 6644: case 7507: case 8454:  strValue="san";  break;
	case 6290: case 7763: case 8210:  strValue="sang";  break;
	case 6003: case 7150: case 7156: case 7593: case 8094: case 8694:  strValue="sao";  break;
		strValue="se";  break;
		strValue="sen";  break;
		strValue="seng";  break;
	case 6394: case 7606: case 7901: case 8080: case 8436: case 8614: case 8672:  strValue="sha";  break;
	case 8507:  strValue="shai";  break;
	case 5663: case 5808: case 5923: case 5979: case 6047: case 6890: case 7009: case 7051: case 7083: case 7594: case 7844: case 8062: case 8321: case 8414: case 8539: case 8713:  strValue="shan";  break;
	case 5980: case 7120: case 7368: case 7656: case 8592:  strValue="shang";  break;
	case 5931: case 6070: case 6891: case 7228: case 8366: case 8425:  strValue="shao";  break;
	case 5639: case 5760: case 6606: case 6860: case 7608: case 7820: case 8774:  strValue="she";  break;
	case 5837: case 6123: case 6351: case 6841: case 7309: case 7547: case 7982: case 8255:  strValue="shen";  break;
	case 6551: case 7441: case 7782: case 8347:  strValue="sheng";  break;
	case 5854: case 5985: case 6110: case 6173: case 6317: case 7388: case 7459: case 7634: case 7870: case 8307: case 8334: case 8363: case 8525: case 8669: case 8685:  strValue="shi";  break;
	case 6587: case 7123: case 8428:  strValue="shou";  break;
	case 5731: case 5951: case 6136: case 6283: case 6780: case 6888: case 7013: case 7508: case 7582: case 7988:  strValue="shu";  break;
	case 6407:  strValue="shua";  break;
	case 8316:  strValue="shuai";  break;
	case 6737: case 6844:  strValue="shuan";  break;
	case 7055:  strValue="shuang";  break;
		strValue="shui";  break;
		strValue="shun";  break;
	case 6184: case 6287: case 6989: case 7335: case 7869:  strValue="shuo";  break;
	case 5643: case 5778: case 5944: case 6348: case 6765: case 6784: case 6889: case 7006: case 7065: case 7133: case 7675: case 7940: case 8024: case 8174: case 8247: case 8351:  strValue="si";  break;
	case 5801: case 6131: case 6534: case 6552: case 6676: case 6704: case 6833: case 8121:  strValue="song";  break;
	case 5937: case 6220: case 6418: case 6453: case 6640: case 6849: case 7612: case 7804: case 7943: case 8284:  strValue="sou";  break;
	case 5777: case 5853: case 6188: case 6428: case 6726: case 6819: case 8389: case 8602: case 8653:  strValue="su";  break;
	case 6601:  strValue="suan";  break;
	case 5839: case 6120: case 6901: case 6968: case 7661: case 7785: case 7801:  strValue="sui";  break;
	case 6105: case 6588: case 6624: case 7330: case 8632:  strValue="sun";  break;
	case 6379: case 6434: case 6442: case 7022: case 7288: case 7792: case 8440:  strValue="suo";  break;
	case 6743: case 6866: case 6961: case 7329: case 7719: case 7872: case 8533: case 8703:  strValue="ta";  break;
	case 5902: case 6223: case 6330: case 7070: case 7536: case 7638: case 7849: case 8544: case 8656:  strValue="tai";  break;
	case 5916: case 6903: case 7428: case 7694: case 7867: case 7936: case 8191:  strValue="tan";  break;
	case 5746: case 6491: case 6871: case 7209: case 7344: case 7906: case 7959: case 8177: case 8305: case 8311: case 8442: case 8517:  strValue="tang";  break;
	case 5627: case 6391: case 6812: case 7226: case 7666:  strValue="tao";  break;
	case 6315: case 7693: case 7911:  strValue="te";  break;
	case 7588:  strValue="teng";  break;
	case 5735: case 6709: case 6949: case 7130: case 8035: case 8151: case 8514:  strValue="ti";  break;
	case 6261: case 6735: case 6757: case 7369: case 7817:  strValue="tian";  break;
	case 5712: case 7686: case 8127: case 8272: case 8352: case 8448: case 8622: case 8670: case 8756:  strValue="tiao";  break;
	case 6138: case 8749:  strValue="tie";  break;
	case 6080: case 6167: case 7035: case 7272: case 7890: case 8249: case 8610:  strValue="ting";  break;
	case 5701: case 5758: case 6077: case 6444: case 6690: case 6892: case 7737:  strValue="tong";  break;
	case 7855: case 7822: case 8727:  strValue="tou";  break;
	case 6002: case 6117: case 6143: case 7842: case 8509:  strValue="tu";  break;
	case 6250: case 6972:  strValue="tuan";  break;
	case 7653:  strValue="tui";  break;
	case 5759: case 6629: case 7453: case 7564:  strValue="tun";  break;
	case 5617: case 5702: case 5971: case 6653: case 6791: case 7256: case 7262: case 7350: case 7740: case 8374: case 8502: case 8541: case 8630:  strValue="tuo";  break;
	case 5684: case 7020: case 7580:  strValue="wa";  break;
		strValue="wai";  break;
	case 5664: case 6025: case 6150: case 7093: case 7126: case 7194: case 7568: case 7821: case 8274:  strValue="wan";  break;
	case 5672: case 6244: case 6715: case 7394: case 8745:  strValue="wang";  break;
	case 5743: case 5835: case 5881: case 5883: case 6158: case 6217: case 6488: case 6501: case 6543: case 6545: case 6611: case 6612: case 6739: case 6777: case 6802: case 6822: case 6952: case 7024: case 7166: case 7224: case 7406: case 7631: case 7648: case 8084: case 8426: case 8659:  strValue="wei";  break;
	case 5656: case 6751: case 6775: case 7223: case 8609:  strValue="wen";  break;
	case 6178: case 6219:  strValue="weng";  break;
	case 5733: case 6111: case 6502: case 6855: case 7531: case 7750: case 8627:  strValue="wo";  break;
	case 5603: case 5685: case 5867: case 5889: case 5956: case 6044: case 6377: case 6648: case 6668: case 6672: case 6820: case 6927: case 6935: case 6992: case 7036: case 7080: case 7227: case 7485: case 7641: case 8036: case 8045: case 8077: case 8258: case 8640: case 8789:  strValue="wu";  break;
	case 5750: case 5766: case 5884: case 5913: case 6130: case 6163: case 6191: case 6241: case 6381: case 6567: case 6630: case 6750: case 6827: case 6832: case 6979: case 7050: case 7184: case 7356: case 7456: case 7474: case 7604: case 7668: case 7689: case 7691: case 8010: case 8122: case 8265: case 8303: case 8312: case 8410: case 8424: case 8443: case 8449: case 8466: case 8521: case 8791:  strValue="xi";  break;
	case 6340: case 6582: case 6958: case 7206: case 7252: case 7744: case 8093: case 8333: case 8779:  strValue="xia";  break;
	case 5794: case 5823: case 6040: case 6118: case 6226: case 6513: case 6593: case 6963: case 7021: case 7515: case 7662: case 7676: case 8034: case 8079: case 8225: case 8358: case 8444: case 8503: case 8548: case 8549: case 8617:  strValue="xian";  break;
	case 6028: case 6157: case 6635: case 6652: case 7088: case 7129: case 8313: case 8663: case 8747:  strValue="xiang";  break;
	case 6356: case 6537: case 6876: case 6948: case 7071: case 7115: case 7241: case 7253: case 8257: case 8367: case 8379: case 8744:  strValue="xiao";  break;
	case 5741: case 5784: case 5936: case 5938: case 6215: case 6302: case 6619: case 6661: case 6845: case 6912: case 6966: case 7105: case 7151: case 7331: case 7339: case 8583:  strValue="xie";  break;
	case 5622: case 6016: case 7431: case 7607: case 8646:  strValue="xin";  break;
	case 5874: case 6084: case 6309: case 6712: case 7742:  strValue="xing";  break;
	case 6026:  strValue="xiong";  break;
	case 6361: case 6522: case 6642: case 6651: case 6869: case 8028: case 8587: case 8759:  strValue="xiu";  break;
	case 5828: case 5935: case 5955: case 6203: case 6810: case 6851: case 7179: case 7282: case 7667: case 7776: case 8167: case 8458: case 8515:  strValue="xu";  break;
	case 5756: case 5846: case 6170: case 6279: case 6789: case 6854: case 6886: case 7215: case 7324: case 7449: case 7637: case 7651: case 7759: case 7871: case 7964: case 8071:  strValue="xuan";  break;
	case 5842: case 7720: case 8529: case 8708:  strValue="xue";  break;
	case 5767: case 5908: case 5987: case 6087: case 6101: case 6206: case 6225: case 6530: case 6563: case 6620: case 6694: case 6813: case 6817: case 7454: case 8131: case 8524: case 8664:  strValue="xun";  break;
	case 5683: case 5975: case 6275: case 6512: case 6934: case 7011: case 7180: case 7266: case 7518: case 7728: case 7793: case 8073:  strValue="ya";  break;
	case 5641: case 5645: case 5718: case 5740: case 5780: case 5861: case 5917: case 5919: case 6030: case 6146: case 6535: case 6691: case 6738: case 6753: case 6846: case 6857: case 6991: case 7044: case 7192: case 7360: case 7444: case 7557: case 7645: case 7827: case 8359: case 8506: case 8742: case 8748: case 8790:  strValue="yan";  break;
	case 6564: case 6683: case 7630: case 7640: case 7706: case 8253: case 8717:  strValue="yang";  break;
	case 5618: case 5619: case 6326: case 6542: case 6570: case 7159: case 7182: case 7235: case 7387: case 7455: case 7540: case 7902: case 8046: case 8126: case 8477: case 8705:  strValue="yao";  break;
	case 5644: case 5843: case 5894: case 6262: case 7442: case 7639: case 7884:  strValue="ye";  break;
	case 5655: case 5657: case 5670: case 5693: case 5711: case 5817: case 5961: case 5992: case 6018: case 6051: case 6072: case 6218: case 6236: case 6240: case 6258: case 6314: case 6329: case 6355: case 6362: case 6441: case 6470: case 6527: case 6558: case 6602: case 6634: case 6688: case 6689: case 6708: case 6884: case 6938: case 7068: case 7143: case 7376: case 7383: case 7461: case 7629: case 7658: case 7784: case 7838: case 7955: case 7978: case 8074: case 8089: case 8115: case 8120: case 8270: case 8415: case 8464: case 8472: case 8493: case 8780:  strValue="yi";  break;
	case 5623: case 5920: case 5983: case 6007: case 6065: case 6337: case 6419: case 6594: case 6625: case 6806: case 7519: case 7887: case 8111: case 8230: case 8615: case 8624:  strValue="yin";  break;
	case 5788: case 5911: case 6067: case 6094: case 6126: case 6151: case 6186: case 6292: case 6451: case 6663: case 6862: case 6875: case 6913: case 7188: case 7212: case 7326: case 7584: case 8048: case 8108: case 8203: case 8331:  strValue="ying";  break;
	case 6401:  strValue="yo";  break;
	case 5724: case 5953: case 6013: case 6415: case 6728: case 7163: case 7962: case 8014: case 8711: case 8751:  strValue="yong";  break;
	case 5653: case 5692: case 5707: case 6112: case 6115: case 6121: case 6347: case 6483: case 6922: case 7254: case 7364: case 7527: case 7880: case 8064: case 8236: case 8242: case 8286: case 8647: case 8778: case 8788:  strValue="you";  break;
	case 5614: case 5625: case 5681: case 5722: case 5836: case 5845: case 6139: case 6187: case 6277: case 6484: case 6486: case 6546: case 6592: case 6632: case 6637: case 6655: case 6748: case 6987: case 6993: case 7005: case 7090: case 7204: case 7437: case 7476: case 7573: case 7603: case 7622: case 7647: case 7659: case 7718: case 7858: case 8033: case 8054: case 8085: case 8086: case 8130: case 8133: case 8266: case 8285: case 8336: case 8407: case 8408: case 8607: case 8625:  strValue="yu";  break;
	case 5989: case 6011: case 6282: case 6768: case 7034: case 7205: case 7358: case 7528: case 7783: case 8016: case 8302: case 8378: case 8629:  strValue="yuan";  break;
	case 5763: case 6914: case 7348: case 7530: case 7865:  strValue="yue";  break;
	case 5909: case 6031: case 6581: case 6702: case 6719: case 7101: case 7225: case 7370: case 7432: case 7521: case 7657:  strValue="yun";  break;
	case 6257: case 6338:  strValue="za";  break;
	case 6544: case 7162:  strValue="zai";  break;
	case 7222: case 7435: case 8402: case 8456: case 8485: case 8641:  strValue="zan";  break;
	case 6242: case 7064: case 7416:  strValue="zang";  break;
	case 6380:  strValue="zao";  break;
	case 5638: case 8369: case 5651: case 6385: case 6493: case 6937: case 7430: case 8348: case 8423:  strValue="ze";  break;
		strValue="zei";  break;
	case 5858:  strValue="zen";  break;
	case 7153: case 7421: case 7832: case 7913:  strValue="zeng";  break;
	case 6610: case 6274: case 6324: case 6369: case 6378: case 7736: case 8068: case 8238: case 8794:  strValue="zha";  break;
	case 7746: case 8109:  strValue="zhai";  break;
	case 5862: case 6288: case 7625:  strValue="zhan";  break;
	case 5675: case 5921: case 6504: case 6554: case 6615: case 7049: case 7216: case 8315:  strValue="zhang";  break;
	case 5815: case 7294: case 7840: case 8341:  strValue="zhao";  break;
	case 5856: case 6301: case 7247: case 7392: case 7761: case 8049: case 8162: case 8256: case 8487:  strValue="zhe";  break;
	case 5958: case 6172: case 6805: case 7139: case 7269: case 7327: case 7384: case 7466: case 7551: case 7562: case 7685: case 7819: case 8001: case 8018: case 8380:  strValue="zhen";  break;
	case 5826: case 6531: case 6571: case 7859: case 7903: case 8361:  strValue="zheng";  break;
	case 5620: case 5876: case 5904: case 5990: case 6038: case 6293: case 6489: case 6669: case 6973: case 6975: case 7079: case 7246: case 7255: case 7257: case 7268: case 7382: case 7389: case 7462: case 7553: case 7589: case 7677: case 7683: case 7773: case 7984: case 8026: case 8075: case 8246: case 8474: case 8505: case 8537: case 8557: case 8560: case 8584: case 8603:  strValue="zhi";  break;
	case 5803: case 7981: case 8314: case 8417: case 8564:  strValue="zhong";  break;
	case 6107: case 6390: case 7008: case 7091: case 7107: case 7548: case 7756: case 8406: case 8492:  strValue="zhou";  break;
	case 5689: case 5710: case 5905: case 6049: case 6079: case 6808: case 6830: case 6883: case 7244: case 7338: case 7345: case 7636: case 7889: case 8070: case 8081: case 8335: case 8371: case 8422: case 8467: case 8578: case 8770:  strValue="zhu";  break;
		strValue="zhua";  break;
		strValue="zhuai";  break;
	case 6389: case 6645: case 8207:  strValue="zhuan";  break;
	case 5755:  strValue="zhuang";  break;
	case 6723: case 7077: case 7136:  strValue="zhui";  break;
	case 7538: case 8124:  strValue="zhun";  break;
	case 5730: case 5834: case 6310: case 6823: case 6835: case 6910: case 7644: case 7690: case 7729: case 7977:  strValue="zhuo";  break;
	case 5849: case 6549: case 7002: case 7060: case 7127: case 7287: case 7402: case 7463: case 7707: case 7786: case 7937: case 7986: case 8172: case 8342: case 8450: case 8484: case 8594: case 8604: case 8623: case 8686: case 8758:  strValue="zi";  break;
	case 5744: case 7574: case 8453:  strValue="zong";  break;
	case 5833: case 5878: case 5924: case 7067: case 8677:  strValue="zou";  break;
	case 5762: case 6147: case 7963:  strValue="zu";  break;
	case 6312: case 7158: case 8582:  strValue="zuan";  break;
	case 6209:  strValue="zui";  break;
	case 6304: case 7355: case 8714:  strValue="zun";  break;
	case 5872: case 6382: case 6460: case 6684: case 7549: case 7681:  strValue="zuo";  break;
	default:strValue="?";break;
	}
}

void get_full_pinyin(unsigned char* cn, std::string& py,bool upper)
{
	int chinese_str_len = strlen((char*)cn);
	for(int i = 0; i < chinese_str_len; i++)
	{
		if(cn[i] > 0 && cn[i] < 160)
		{
			py += cn[i];
		}
		else
		{
			//int ascii_code = Chinese[i]*256 + Chinese[i+1] - 256*256;
			int ascii_code = (cn[i] - 0xa0)*100 + cn[i+1] - 0xa0;
			++i;
			if (ascii_code < 1601) //未知字符
			{ 
				py += "?";
				continue;
			}

			if(ascii_code > 5589)//OtherCode
			{
				std::string s;
				get_other_code_py(ascii_code, s);
				py += s;
				continue;
			}

			for (int j = 396; j >= 0; j--) //区位码
			{
				if (ascii_code >= code_qw[j])
				{
					py += pinyin[j];
					break;
				}
			}
		}
	}

	if (upper)	upper_nc(py);
}

bool to_bool(const std::string& v) {
    if (v == "true" || v == "1") return true;
    if (v == "false" || v == "0") return false;

    throw std::string("invalid value for bool");
}

double to_double(const std::string& v) {
    char* end = NULL;
    double x = ::strtod(v.c_str(), &end);

    if (errno == ERANGE && (x == HUGE_VAL || x == -HUGE_VAL)) {
        errno = 0;
        throw std::string("out of range for double");
    }

    if (end != v.c_str() + v.size()) {
        throw std::string("invalid value for double");
    }

    return x;
}


//c++98 不支持列表初始化，只能采用折中的方式
std::pair<char,int> pairArray[] =   
{  
	std::make_pair('k', 10),  
	std::make_pair('m', 20),  
	std::make_pair('g', 30),
	std::make_pair('t', 40),
	std::make_pair('p', 50)
};  
static ascii_table __int_units(std::map<char,int>(pairArray,pairArray+sizeof(pairArray)/sizeof(pairArray[0])));

//c++11 支持列表初始化
//static ascii_table __int_units {
//	std::map<char, int> {
//		{'k', 10}, {'m', 20}, {'g', 30}, {'t', 40}, {'p', 50},
//	}
//};




int32 to_int32(const std::string& v) {
    int64 x = to_int64(v);

    if (x > MAX_INT32 || x < MIN_INT32) {
        throw std::string("out of range for int32");
    }

    return x;
}

uint32 to_uint32(const std::string& v) {
    int64 x = to_uint64(v);

    if (abs(x) > MAX_UINT32) {
        throw std::string("out of range for uint32");
    }

    return x;
}

int64 to_int64(const std::string& v) {
    if (v.empty()) return 0;

    char c = ::tolower(*v.rbegin());

    if (!__int_units.has(c)) {
        char* end = NULL;
        int64 x = ::strtoll(v.c_str(), &end, 0);

        if (errno == ERANGE && (x == MIN_INT64 || x == MAX_INT64)) {
            errno = 0;
            goto range_err;
        }

        if (end == v.c_str() + v.size()) return x;

        throw std::string("invalid value for integer");

    } else {
        int64 x = to_int64(v.substr(0, v.size() - 1));
        if (x == 0) return 0;

        int off = __int_units.get(c);
        if (x < (MIN_INT64 >> off) || x > (MAX_INT64 >> off)) goto range_err;
        return x << off;
    }

    range_err:
    throw std::string("out of range for int64");
}

uint64 to_uint64(const std::string& v) {
    if (v.empty()) return 0;

    char c = ::tolower(*v.rbegin());

    if (!__int_units.has(c)) {
        char* end = NULL;
        int64 x = ::strtoull(v.c_str(), &end, 0);

        if (errno == ERANGE && (uint64) x == MAX_UINT64) {
            errno = 0;
            goto range_err;
        }

        if (end == v.c_str() + v.size()) return x;

        throw std::string("invalid value for integer");

    } else {
        int64 x = to_uint64(v.substr(0, v.size() - 1));
        if (x == 0) return 0;

        int off = __int_units.get(c);
        if (abs(x) > (MAX_UINT64 >> off)) goto range_err;
        return x << off;
    }

    range_err:
    throw std::string("out of range for uint64");
}

static std::string wchar2str(const wchar_t* wstr, ::size_t wlen) {
    ::iconv_t cd = ::iconv_open("UTF-8", "WCHAR_T");
    if (cd == (::iconv_t) -1) return std::string();

    char* in = (char*) wstr;
    ::size_t in_size = wlen * sizeof(wchar_t);
    ::size_t out_size = wlen * 6;

    std::string s;
    s.resize(out_size);
    char* out = &s[0];

    ::size_t ret = ::iconv(cd, &in, &in_size, &out, &out_size);
    (ret != (::size_t) -1) ? s.resize(s.size() - out_size) : s.clear();

    ::iconv_close(cd);
    return s;
}

std::string to_string(const wchar_t* wstr) {
    return wchar2str(wstr, ::wcslen(wstr));
}

std::string to_string(const std::wstring& wstr) {
    return wchar2str(wstr.c_str(), wstr.size());
}

} // namespace str
