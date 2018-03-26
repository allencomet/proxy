#include "ccflag.h"

#include "../cclog/cclog.h"
#include "../string_util.h"
#include "../thread_util.h"
#include "../os.h"

#include <stdlib.h>
#include <string.h>
#include <memory>
#include <map>


#include <boost/bind.hpp>
#include <boost/smart_ptr.hpp>

DEF_string(config, "", "path of config file");

namespace ccflag {
namespace xx {

struct FlagInfo {
    FlagInfo(const char* _type_str, const char* _name, const char* _value,
             const char* _help, const char* _file, void* _addr, int _type)
        : type_str(_type_str), name(_name), value(_value), help(_help),
          file(_file), addr(_addr), type(_type) {
    }

    std::string to_string() const {
        return std::string("--") + name + ": " + help +
            "\n\t type" + ": " + type_str + "\t     default" + ": " + value +
            "\n\t from" + ": " + file;
    }

    const char* const type_str;
    const char* const name;
    const char* const value;    // default value
    const char* const help;
    const char* const file;     // file where the flag is defined
    void* const addr;           // pointer to the flag variable
    int type;
};

class Flagger {
  public:
    static Flagger* instance() {
        static Flagger kFlagger;
        return &kFlagger;
    }

    ~Flagger() {
        if (_conf_thread != NULL) _conf_thread->join();
    }

    void add_flag(const char* type_str, const char* name, const char* value,
                  const char* help, const char* file, void* addr, int type);

    bool set_flag_value(const std::string& flg, const std::string& val,
                        std::string& err);

    bool set_bool_flags(const std::string& flg, std::string& err);

    std::string flags_info_to_string() const;

    std::vector<std::string>
    parse_flags_from_command_line(int argc, char** argv);

    void parse_flags_from_config(const std::string& config);

    void start_conf_thread() {
        _conf_thread.reset(
            new safe::StoppableThread(boost::bind(&Flagger::thread_fun, this), 3000));
        _conf_thread->start();
    }

  private:
    std::map<std::string, FlagInfo> _map;
	//std::unique_ptr<StoppableThread> _conf_thread;
	boost::scoped_ptr<safe::StoppableThread> _conf_thread;
    os::file _config;

	Flagger() {};
    void thread_fun();
};

void Flagger::add_flag(const char* type_str, const char* name,
                       const char* value, const char* help, const char* file,
                       void* addr, int type) {
	std::pair<std::map<std::string, FlagInfo>::iterator, bool> ret = _map.insert(
        std::make_pair(name, FlagInfo(type_str, name, value, help, file, addr,
                                      type)));

    FLOG_IF(!ret.second) << "flags defined with the same name" << ": " << name
        << ", from " << ret.first->second.file << ", and " << file;
}

//设置配置项对应的值
bool Flagger::set_flag_value(const std::string& flg, const std::string& v,
                             std::string& err) {
	std::map<std::string, FlagInfo>::iterator it = _map.find(flg);//寻找该配置项
    if (it == _map.end()) {
        err = std::string("flag not defined") + ": " + flg;
        return false;
    }

	//如果类型转换异常可以通过try来捕获，避免程序崩溃
    try {
		FlagInfo& fi = it->second;
        switch (fi.type) {
          case TYPE_string:
            *static_cast<std::string*>(fi.addr) = v;
            break;

          case TYPE_bool:
            *static_cast<bool*>(fi.addr) = str::to_bool(v);
            break;

          case TYPE_int32:
            *static_cast<int32*>(fi.addr) = str::to_int32(v);
            break;

          case TYPE_uint32:
            *static_cast<uint32*>(fi.addr) = str::to_uint32(v);
            break;

          case TYPE_int64:
            *static_cast<int64*>(fi.addr) = str::to_int64(v);
            break;

          case TYPE_uint64:
            *static_cast<uint64*>(fi.addr) = str::to_uint64(v);
            break;

          case TYPE_double:
            *static_cast<double*>(fi.addr) = str::to_double(v);
            break;
        }

    } catch (std::string& e) {
        err = e + ": " + v;
        return false;
    }

    return true;
}

/*
 * set_bool_flags("abc", err);
 *
 *   --abc ==> true
 *   --a, --b, --c ==> true
 */
bool Flagger::set_bool_flags(const std::string& flg, std::string& err) {
	std::map<std::string, FlagInfo>::iterator it = _map.find(flg);//寻找该配置项
    if (it != _map.end()) {
        if (it->second.type != TYPE_bool) {//如果该配置项不是二值含义的选项报错返回
            err = std::string("value not set for non-bool flag") + ": " + flg;
            return false;
        }

        *static_cast<bool*>(it->second.addr) = true;//开启该选项的值
        return true;
    }

	//将a、b、c值设置成true
    for (::size_t i = 0; i < flg.size(); ++i) {
        it = _map.find(flg.substr(i, 1));
        if (it == _map.end()) {
            err = std::string("invalid combination of bool flags") + ": " + flg;
            return false;
        }

        *static_cast<bool*>(it->second.addr) = true;
    }

    return true;
}

std::string Flagger::flags_info_to_string() const {
    std::string s;
    for (std::map<std::string, FlagInfo>::const_iterator it = _map.begin(); it != _map.end(); ++it) {
        if (it->second.help[0] != '\0') {
            s += it->second.to_string() + "\n";
        }
    }
    return s;
}

FlagSaver::FlagSaver(const char* type_str, const char* name, const char* value,
                     const char* help, const char* file, void* addr, int type) {
    Flagger::instance()->add_flag(type_str, name, value, help, file, addr, type);
}

static std::string trim_string(const std::string& s) {
    std::string x = str::replace(s, "'\"\t\r\n", ' ');
    x = str::replace(x, "　", " ");  // replace Chinese space
    return str::strip(x);
}

static std::string getline(os::file& ifs) {
    std::string line;

	for (;;) {
        std::string s = xx::trim_string(ifs.getline());
        if (ifs.eof()) return line;

        if (s.empty() || *s.rbegin() != '\\') {
            line += s;
            return line;
        }

        s.resize(s.size() - 1);
        line += xx::trim_string(s);
    }
}

std::vector<std::string>
Flagger::parse_flags_from_command_line(int argc, char** argv) {
    if (argc <= 1) return std::vector<std::string>();

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);//收集参数列表(从程序名后参数开始)
    }

    /*
     * ./exe --  or  ./exe --help     print flags info and exit
     */
    if (args.size() == 1 && (args[0].find_first_not_of('-') == std::string::npos //如果第一个参数以横杠结尾，则显示帮助信息然后退出
        || args[0] == "--help")) {//如果第一个参数是--help，则显示帮助信息然后退出
        printf("%s", this->flags_info_to_string().c_str());
        exit(0);
    }

    std::string flg, val;
    std::vector<std::string> v;

	//遍历所有参数
    for (::size_t i = 0; i < args.size(); ++i) {
		std::string& arg = args[i];

        if (arg[0] != '-') {
            v.push_back(arg);  // non-flag element(跳过不符合格式的参数,参数必须以-开始)
            continue;
        }

		//有效参数必须是：-file_name=allen 或者--file_name=allen，开头的-不计数量
        ::size_t bp = arg.find_first_not_of('-');
        ::size_t ep = arg.find('=');
        FLOG_IF(ep <= bp) << "invalid parameter" << ": " << arg;

        std::string err;

        if (ep == std::string::npos) {//开启选项操作，并非赋值操作
            flg = arg.substr(bp);//例如-verbose,那么flag就是verbose
            if (!this->set_bool_flags(flg, err)) FLOG << err;//开启verbose选项
        } else {
            flg = arg.substr(bp, ep - bp);//例如-file_name=allen,flag为file_name
            val = arg.substr(ep + 1);//val为allen
            if (!this->set_flag_value(flg, val, err)) FLOG << err;//设置file_name值为allen
        }
    }

    return v;
}

//解析配置文件xx.conf，从配置文件读取配置项
void Flagger::parse_flags_from_config(const std::string& config) {
    if (!_config.open(config, "r")) {
        FLOG << "failed to open config file" << ": " << config;
    }

    while (!_config.eof()) {
        std::string line = xx::trim_string(xx::getline(_config));
        if (line.empty() || line[0] == '#' || line.substr(0, 2) == "//") {
            continue;
        }

        ::size_t pos = std::min(line.find('#'), line.find("//"));
        if (pos != std::string::npos) line.resize(pos);

        pos = line.find('=');
        if (pos + 1 <= 1) {
            FLOG << "invalid config: " << line << ", at "
                 << _config.path() << ':' << _config.lineno();
        }

        /*
         * ignore '!' at the beginning of line
         */
        int x = (line[0] == '!');
        std::string flg = xx::trim_string(line.substr(x, pos - x));
        std::string val = xx::trim_string(line.substr(pos + 1));

        std::string err;
        if (!xx::Flagger::instance()->set_flag_value(flg, val, err)) {
            FLOG << err << ", at " << _config.path() << ':' << _config.lineno();
        }
    }
}

void Flagger::thread_fun() {
    if (!_config.exists() || !_config.modified()) return;//如果配置文件不存在并且没有被修改过，则不重新读取

    if (!_config.open()) {
        ELOG << "failed to open config file: " << _config.path();
        return;
    }

    while (!_config.eof()) {
        std::string line = xx::trim_string(xx::getline(_config));

        // ignore lines not beginning with '!'
        if (line.empty() || line[0] != '!') continue;

        ::size_t pos = std::min(line.find('#'), line.find("//"));
        if (pos != std::string::npos) line.resize(pos);

        pos = line.find('=');
        if (pos == std::string::npos) {
            ELOG << "invalid config: " << line << ", at "
                 << _config.path() << ':' << _config.lineno();
            continue;
        }

        std::string flg = xx::trim_string(line.substr(1, pos - 1));
        std::string val = xx::trim_string(line.substr(pos + 1));

        std::string err;
        if (!xx::Flagger::instance()->set_flag_value(flg, val, err)) {
            ELOG << err << ", at " << _config.path() << ':' << _config.lineno();
        }
    }
}

}  // namespace xx

std::vector<std::string> init_ccflag(int argc, char** argv) {
	xx::Flagger *ins = xx::Flagger::instance();//单例
	std::vector<std::string> v = ins->parse_flags_from_command_line(argc, argv);

    if (!::FLG_config.empty()) {
        ins->parse_flags_from_config(::FLG_config);//从配置文件中解析配置选项
        ins->start_conf_thread();//动态检测配置文件是否更新，更新则重新读取
    }

    return v;
}

std::string set_flag_value(const std::string& name, const std::string& value) {
    std::string err;
    xx::Flagger::instance()->set_flag_value(name, value, err);
    return err;
}

}  // namespace ccflag
