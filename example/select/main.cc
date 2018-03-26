// Copyright (c) 2015
// Author: Chrono Law
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <deque>
#include <list>
#include <string>
#include <algorithm>
#include <assert.h>
#include <typeinfo>


#include "../rlog/rlog.h"
#include <boost/bind.hpp>


using namespace boost;
//using namespace placeholders;

 /*
using placeholders::_1;
using placeholders::_2;
using placeholders::_3;
using placeholders::_4;
using placeholders::_5;
using placeholders::_6;
using placeholders::_7;
using placeholders::_8;
using placeholders::_9;
*/



int f(int a, int b)
{
	return a + b;
}

int g(int a, int b, int c)
{
	return a + b * c;
}

typedef int(*f_type)(int, int);
typedef int(*g_type)(int, int, int);
//typedef decltype(&f) f_type;      //since c++11
//typedef decltype(&g) g_type;      //since c++11


//自由函数
void bind_free_functor()
{
	std::cout << boost::bind(f, 1, 2)() << std::endl;
	std::cout << boost::bind(g, 1, 2, 3)() << std::endl;

	int x = 1, y = 2, z = 3;
	int ret = 0;

	std::cout << " int f(int a, int b) { return a + b; } " << std::endl;
	std::cout << " int g(int a, int b, int c) { return a + b * c;} " << std::endl;

	ret = boost::bind(f, _1, 9)(x);
	std::cout << "x = 1, y = 2, z = 3: bind(f, _1,  9)(x) : " << ret << std::endl;
	ret = boost::bind(f, _1, _2)(x, y);
	std::cout << "x = 1, y = 2, z = 3: bind(f, _1, _2)(x, y) : " << ret << std::endl;
	ret = boost::bind(f, _2, _1)(x, y);
	std::cout << "x = 1, y = 2, z = 3: bind(f, _2, _1)(x, y) : " << ret << std::endl;
	ret = boost::bind(f, _1, _1)(x, y);
	std::cout << "x = 1, y = 2, z = 3: bind(f, _1, _1)(x, y) : " << ret << std::endl;
	ret = boost::bind(g, _1, 8, _2)(x, y);
	std::cout << "x = 1, y = 2, z = 3: bind(g, _1, 8, _2)(x, y) : " << ret << std::endl;
	ret = boost::bind(g, _3, _2, _2)(x, y, z);
	std::cout << "x = 1, y = 2, z = 3: bind(g, _3, _2, _2)(x, y, z) : " << ret << std::endl;

	f_type pf = f;
	g_type pg = g;

	std::cout << "x = 1, y = 2, z = 3: bind(pf, _1, 9)(x) : " << boost::bind(pf, _1, 9)(x) << std::endl;
	std::cout << "x = 1, y = 2, z = 3: bind(pg, _3, _2, _2)(x, y, z) : " << boost::bind(pg, _3, _2, _2)(x, y, z) << std::endl;
}


class demo
{
public:
	int f(int a, int b) { return a + b; }
};

class point
{
public:
	int x, y;

	point(int a = 0, int b = 0) :x(a), y(b) {}
	void print() { std::cout << "(" << x << "," << y << ")\n"; }
};


//成员函数
void bind_member_functor()
{
	demo a;
	demo &ra = a;
	demo *p = &a;

	std::cout << boost::bind(&demo::f, a, _1, 20)(10) << std::endl;
	std::cout << boost::bind(&demo::f, ra, _2, _1)(10, 20) << std::endl;
	std::cout << boost::bind(&demo::f, p, _1, _2)(10, 20) << std::endl;

	std::vector<point> v(10);
	std::for_each(v.begin(), v.end(), boost::bind(&point::print, _1));
}



//成员变量  
void bind_class_member()
{
	std::vector<point> v(10);
	std::vector<int> v2(10);

	std::transform(v.begin(), v.end(), v2.begin(), boost::bind(&point::x, _1));

	for (std::vector<int>::iterator it = v2.begin(); it != v2.end(); ++it)
		std::cout << *it << ",";

	typedef std::pair<int, std::string> pair_t;
	pair_t p(123, "string");

	std::cout << boost::bind(&pair_t::first, p)() << std::endl;
	std::cout << boost::bind(&pair_t::second, p)() << std::endl;

}


//函数对象 
class functor_01
{
public:
	int operator()(int a, int b)
	{
		return a + b;
	}
};

class functor_02
{
public:
	double operator()(double a, double b)
	{
		return a + b;
	}
};

class functor_03
{
public:
	std::string operator()(std::string str)
	{
		return str;
	}
};

// 4. 函数对象  
void test_bind_functor()
{
	boost::bind(std::greater<int>(), _1, 10);
	boost::bind(std::plus<int>(), _1, _2);
	boost::bind(std::modulus<int>(), _1, 3);

	std::cout << boost::bind<int>(functor_01(), _1, _2)(10, 20) << std::endl; //bind后面的int是函数对象的返回值


	std::cout << boost::bind(std::plus<int>(), _1, _2)(10, 20) << std::endl;
	std::cout << boost::bind(std::modulus<int>(), _1, 3)(11) << std::endl;
	std::cout << std::boolalpha << boost::bind(std::greater<int>(), _1, 10)(20) << std::endl;
	std::cout << boost::bind<double>(functor_02(), _1, _2)(11.0, 22.0) << std::endl;    //bind后面那个double是指该函数对象的返回类型
	std::cout << boost::bind<std::string>(functor_03(), _1)("functor") << std::endl;    //bind后面那个double是指该函数对象的返回类型
}



// 可以使用ref库包装了对象的引用，可以让bind 存储对象引用的拷贝，从而降低了拷贝的代价
void bind_ref()
{
	int x = 10;
	std::cout << boost::bind(g, _1, boost::cref(x), boost::ref(x))(10) << std::endl;

	functor_01 af;
	std::cout << boost::bind<int>(boost::ref(af), _1, _2)(10, 20) << std::endl;

}

/*

void case6()
{
	int x = 10;
	auto r = ref(x);
	{
		int *y = new int(20);
		r = ref(*y);

		std::cout << r << std::endl;
		std::cout << bind(g, r, 1, 1)() << std::endl;
		delete y;
	}
	std::cout << bind(g, r, 1, 1)() << std::endl;
}


#include <boost/rational.hpp>

void case7()
{
	typedef rational<int> ri;
	std::vector<ri> v = {ri(1,2),ri(3,4),ri(5,6)};


	std::remove_if(v.begin(), v.end(), bind(&ri::numerator, _1) == 1 );
	assert(v[0].numerator() == 3);


	assert(std::find_if(v.begin(), v.end(), bind(&ri::numerator, _1) == 1) == v.end());

	auto pos = std::find_if(v.begin(), v.end(),
			bind(&ri::numerator, _1) >3 && bind(&ri::denominator, _1) < 8);

	std::cout << *pos << std::endl;

	pos = find_if(v.begin(), v.end(),
			[](ri &r)
			{
				return r.numerator() >3 && r.denominator() < 8;
			});
	std::cout << *pos << std::endl;
}

//////////////////////////////////////////

void case8()
{
	auto lf = [](int x)
	{
		return f(x, 9);
	};

	assert(lf(10) == bind(f, _1, 9)(10));
}

//////////////////////////////////////////

int f(double a, double b)
{   return a * b;}

typedef int (*f_type1)(int, int);
typedef int (*f_type2)(double, double);

void case9()
{
	//std::cout << bind(f,1,2)() << std::endl;

	f_type1 pf1 = f;
	f_type2 pf2 = f;

	std::cout << bind(pf1,1,2)() << std::endl;
	std::cout << bind(pf2,1,2)() << std::endl;
}
*/







#include <boost/current_function.hpp>
#include <boost/ref.hpp>

//函数对象
struct square
{
	typedef void result_type;
	result_type operator()(int &x)
	{
		std::cout << (x = x * x) << std::endl;
	}
};


void ref_case1()
{
	std::cout << "=====================" << BOOST_CURRENT_FUNCTION << "======================" << std::endl;
	std::vector<int> v;
	v.push_back(1);
	v.push_back(2);
	v.push_back(3);
	v.push_back(4);
	v.push_back(5);
	std::for_each(v.begin(), v.end(), square());		//使用标准库的for_each算法，c++98也有此函数
}



void ref_case2()
{
	std::cout << "=====================" << BOOST_CURRENT_FUNCTION << "======================" << std::endl;
	int x = 10;
	boost::reference_wrapper<int> rw(x);
	assert(x == rw);
	(int &)rw = 100;
	assert(x == 100);

	boost::reference_wrapper<int> rw2(rw);
	assert(rw2.get() == 100);

	std::string str;
	boost::reference_wrapper<std::string> rws(str);
	*rws.get_pointer() = "test reference_wrapper";
	std::cout << rws.get().size() << std::endl;
}

class GGGE
{
public:
	typedef bool result_type;
	result_type operator()(int x)
	{
		return x > 10 ? true : false;
	}

	void print(int x, int y)
	{
		if (x > y)
		{
			std::cout << x << std::endl;
		}
	}
};


void ref_case3()
{
	std::cout << "=====================" << BOOST_CURRENT_FUNCTION << "======================" << std::endl;
	double x = 2.71828;
	//auto rw = cref(x);  
	boost::reference_wrapper<const double> rw = boost::cref(x);
	std::cout << typeid(rw).name() << std::endl;

	std::string str;
	//auto rws = boost::ref(str);     
	boost::reference_wrapper<std::string>	rws = boost::ref(str);
	std::cout << typeid(rws).name() << std::endl;

	boost::cref(str);   //adl

	std::vector<int> v;
	v.push_back(1);
	v.push_back(2);
	v.push_back(3);
	v.push_back(4);
	v.push_back(5);
	v.push_back(15);
	v.push_back(16);
	v.push_back(17);
	v.push_back(18);
	v.push_back(19);
	v.push_back(20);


	//对自由方法来说，直接boost::bind(函数名, 参数1，参数2，...)
	//对类方法来说，直接boost::bind(&类名::方法名，类实例指针，参数1，参数2）
	GGGE gge;

	std::remove_if(v.begin(), v.end(), GGGE());
	std::for_each(v.begin(), v.end(), boost::bind(&GGGE::print, gge, _1, boost::cref(4)));
}


class big_class
{
private:
	int x;
public:
	big_class() :x(0) {}
	void print()
	{
		std::cout << "big_class " << ++x << std::endl;
	}
};


template<typename T>
void print(T a)
{
	for (int i = 0; i < 2; ++i)
		boost::unwrap_ref(a).print();
}
void case6()
{
	std::cout << "=====================" << BOOST_CURRENT_FUNCTION << "======================" << std::endl;
	big_class c;
	boost::reference_wrapper<big_class> rw = boost::ref(c);
	c.print();
	std::cout << "============1===========" << std::endl;
	print(c);		//拷贝传参，不改变内部状态
	std::cout << "============2===========" << std::endl;
	print(rw);		//引用传参，改变内部状态
	std::cout << "============3===========" << std::endl;
	print(c);		//拷贝传参，不改变内部状态(但是此次传入的参数会受到前一次引用传参的结果影响)
	std::cout << "============4===========" << std::endl;
	c.print();
}



//下面的语句：这是不正确的，因为ref库并没有函数调用功能，只有用c++11的std::ref才可以用
void case7()
{
	std::cout << "=====================" << BOOST_CURRENT_FUNCTION << "======================" << std::endl;
	//using namespace std;

	/*typedef double (*pfunc)(double);
	pfunc pf = std::sqrt;
	cout << boost::ref(pf)(5.0) << endl;  */

	//square sq;
	///int x = 5;
	//boost::ref(sq)(x);
	//std::cout << x << endl;

	/*boost::reference_wrapper<square> rw = boost::ref(sq);

	std::vector<int> v;
	v.push_back(1);
	v.push_back(2);
	v.push_back(3);
	v.push_back(4);
	v.push_back(5);
	v.push_back(15);
	v.push_back(16);
	v.push_back(17);
	v.push_back(18);
	v.push_back(19);
	v.push_back(20);
	std::for_each(v.begin(), v.end(), rw);     */
}


#include <boost/lexical_cast.hpp>

//lexical_cast的大多数简易的转换情况下效率极高，比c的sprintf和c++的stringsteam都高

//如果转换失败，则使用默认值
template <typename L, typename R>
R safe_lexical_cast(L l, R &ra, const R &rb)
{
	if (conversion::try_lexical_convert(l, ra))
	{
		return ra;
	}
	else
	{
		return ra = rb;
	}
}

template <typename L, typename R>
R safe_lexical_cast(L l, R &r)
{
	R defaulr = r;
	if (conversion::try_lexical_convert(l, r))
	{
		return r;
	}
	else
	{
		return defaulr;
	}
}

void lexical_cast_test() 	//lexical_cast，要转换的字符串只能包含数字和小数点
{
	int  x = boost::lexical_cast<int>("100"); 	//进行字面值的转换
	long y = boost::lexical_cast<long>("2000");
	float pai = boost::lexical_cast<float>("3.14159e5");
	double e = boost::lexical_cast<double>("2.71828");
	double r = boost::lexical_cast<double>("1.414,xyz", 5); 	//只转换数字，像遇见标点符号就停止转换

	std::cout << x << y << pai << e << r << std::endl;

	std::string str = boost::lexical_cast<std::string>(456); 	//将数字转换为字符串
	std::cout << str << std::endl;

	std::cout << boost::lexical_cast<std::string>(0.618) << std::endl;
	std::cout << boost::lexical_cast<std::string>(0x10) << std::endl;

	std::cout << boost::lexical_cast<bool>("1") << std::endl; 	//将数字转换为bool值

	int u = 1129;
	assert(!conversion::try_lexical_convert("abcd", u));		//扎u你失败了u就是0
	std::cout << u << std::endl; 	//将数字转换为bool值

	std::cout << safe_lexical_cast("abcd", u, 1129) << std::endl;
	u = 1129;
	std::string strNew = "";
	std::cout << safe_lexical_cast(u, strNew, std::string("")) << std::endl;


	char ch = 'x';
	std::string strCH = "";
	strCH = safe_lexical_cast(ch, strCH, std::string(""));
	std::cout << "字符转换成字符串: " << strCH << " 大小：" << strCH.size() << std::endl;

	ch = '\0';
	strCH = "abcd";
	strCH = safe_lexical_cast(ch, strCH);
	std::cout << "字符转换成字符串: " << strCH << " 大小：" << strCH.size() << std::endl;
	//lexical_cast<int>("0x100"); 	//不能将特定格式的字符串字面值转换为数字
	//lexical_cast<int>("123L");
}

#include <boost/function.hpp>


//自由方法
void do_sum_avg(int values[], int n, int& sum, float& avg)
{
	sum = 0;
	for (int i = 0; i < n; i++)
		sum += values[i];
	avg = (float)sum / n;
}

void test_function01()
{
	// The second parameter should be int[], but some compilers (e.g., GCC)
	// complain about this
	boost::function<void(int*, int, int&, float&)> sum_avg;
	sum_avg = &do_sum_avg;

	int values[5] = { 1, 1, 2, 3, 5 };
	int sum;
	float avg;
	sum_avg(values, 5, sum, avg);

	std::cout << "sum = " << sum << std::endl;
	std::cout << "avg = " << avg << std::endl;
}

//成员函数
class Xxx {
public:
	int foo(int a)
	{
		std::cout << a << std::endl;
		return a;
	}
};
int test_function02()
{
	boost::function<int(Xxx*, int)> f;
	f = &Xxx::foo;
	Xxx x;
	f(&x, 5);
	return 0;
}




void printXX(int a)
{
	std::cout << a << std::endl;
}
typedef boost::function<void(int)> SuccessPrint;
int test_function03()
{
	std::vector<SuccessPrint> printList;
	SuccessPrint printOne = boost::bind(printXX, _1);
	printList.push_back(printOne);
	SuccessPrint printTwo = boost::bind(printXX, _1);
	printList.push_back(printTwo);
	SuccessPrint printThree = boost::bind(printXX, _1);
	printList.push_back(printTwo);
	// do something else
	for (int i = 0; i < printList.size(); ++i)
		printList.at(i)(i);
	return 0;
}





class call_back_obj
{
private:
	int x;
public:
	call_back_obj(int i) :x(i) {}
	void operator()(int i)
	{
		std::cout << "call_back_obj:";
		std::cout << i * x++ << std::endl;
	}
};

class demon_class
{
private:
	typedef boost::function<void(int)>	func_t;
	func_t func;
	int n;
public:
	demon_class(int i) :n(i) {}
	template<typename CallBack>
	void accept(CallBack f)
	{
		func = f;
	}

	void run()
	{
		func(n);
	}
};


class call_back_factory
{
public:
	void call_back_func1(int i)
	{
		std::cout << "call_back_factory1";
		std::cout << i * 2 << std::endl;
	}

	void call_back_func2(int i, int j)
	{
		std::cout << "call_back_factory2";
		std::cout << i*j * 2 << std::endl;
	}
};



void test_function()
{
	demon_class		dc(10);
	call_back_obj cbo(2);
	dc.accept(boost::ref(cbo));
	dc.run();
	dc.run();
}

void test_function2()
{
	demon_class		dc(10);
	call_back_factory cbf;
	dc.accept(boost::bind(&call_back_factory::call_back_func1, cbf, _1));
	dc.run();

	dc.accept(boost::bind(&call_back_factory::call_back_func2, cbf, _1, 5));
	dc.run();
}


#include "../util/util.h"
#include "../util/base/string_util.h"
#include <stdio.h>
#include <string.h>


#define CHECK_PARAMS(condition, ret) {\
	if (!(condition)) {\
		fprintf(stderr, "Error: "#condition"  %d\n", ret);  \
		return ret;\
	}\
}


//#define _CHECK_CHAR(ch)	{ch == '\0' ? "" : #ch}
//#define CHECK_CHAR(ch)	_CHECK_CHAR(ch)
#define CHECK_CHAR(ch)	{ch == '\0' ? ' ' : (char)ch}

#include <sys/time.h> 
/*
 *获得当前的系统时间,返回一个long类型的数据
 */
long getCurrentTime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

std::string xxto_string(const double &d)
{
	char szData[20] = { 0 };
	snprintf(szData, 19, "%lf", d);
	return std::string(szData);
}

std::string xxto_string(const int &n)
{
	char szData[20] = { 0 };
	snprintf(szData, 19, "%d", n);
	return std::string(szData);
}

void thread_fun()
{
	std::string strRsc = "[110019][资产帐号认证流水表记录不存在]\n[相关参数：p_fund_account = 0]";
	std::cout << "befor replace: " << strRsc << std::endl;

	long a = getCurrentTime();
	int nCount = 100000;

	while (nCount-- > 0)
	{
		ERRORLOG("Error Data: allen [%d]", nCount);
		TRACELOG("Trace Date: allen [%d]", nCount);
		//std::cout << str::to_string(0.617) <<std::endl;
		//str::to_string(123456);
		//boost::lexical_cast<std::string>(123456);
		//xxto_string(123456);
		//xxto_string(0.617);	//372 ms
		str::replace(str::replace(strRsc, "\n", ""), " ", "");
		//std::cout<<"replace br and replace blank: "<<str::replace(str::replace(strRsc,"\n","")," ","")<<std::endl;
	}

	long b = getCurrentTime();
	std::cout << b - a << " ms" << std::endl;
}


void TrimLeft(char *str)
{
	if (NULL == str || str[0] == '\0')	return;

	char *p = NULL;
	for (p = str; (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'); p++);

	memmove(str, p, strlen(str) - (p - str));
	str[strlen(str) - (p - str)] = '\0';
}

void TrimRight(char *str)
{
	char        *ptr;

	if (NULL == str || str[0] == '\0')		return;

	ptr = str + strlen(str) - 1;

	while (ptr >= str && (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n'))
	{
		*ptr-- = 0x0;
	}
}

void TrimAll(char *str)
{
	TrimLeft(str);
	TrimRight(str);
}

int GetCurYear() {
	int year;
	time_t t;
	struct tm * timeinfo;
	time(&t);
	timeinfo = localtime(&t);
	year = timeinfo->tm_year + 1900;

	return year;
}

int GetCurMonth() {
	int month;
	time_t t;
	struct tm * timeinfo;
	time(&t);
	timeinfo = localtime(&t);
	month = timeinfo->tm_mon + 1;

	return month;
}

int GetCurDay() {
	int day;
	time_t t;
	struct tm * timeinfo;
	time(&t);
	timeinfo = localtime(&t);
	day = timeinfo->tm_mday;

	return day;
}

int GetCurWeek() {
	int wday;
	time_t t;
	struct tm * timeinfo;
	time(&t);
	timeinfo = localtime(&t);
	wday = timeinfo->tm_wday;

	return wday;
}

//判断是否为周末
bool IsWeekend() {
	int nWeek = GetCurWeek();
	if (6 == nWeek || 0 == nWeek) {
		return true;
	}
	else {
		return false;
	}
}

//利用泰勒公式计算星期几
//w=y+[y/4]+[c/4]-2c+[26(m+1)/10]+d-1 
int GetWeek(int year, int month, int day)
{
	int w;    //星期
	int c;    //世纪-1 YYYY的头两位
	int y;    //年份   YYYY的后两位
	int m;    //月份 >=3 1月 2月看成上年的13月 14月
	int d = day;    //日
	if (month >= 3)
	{
		c = year / 100;
		y = year % 100;
		m = month;
	}
	else
	{
		m = month + 12;
		y = (year - 1) % 100;
		c = (year - 1) / 100;
	}
	w = y + y / 4 + c / 4 - 2 * c + (26 * (m + 1)) / 10 + d - 1;
	w = (w + 700) % 7;
	return w;
}

//平年是：365天，闰年是366天。差别是：平年2月是28天，闰年2月是29天。
int GetDaysInMonth(int y, int m) {
	int d;
	int day[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	if (2 == m) {
		d = (((0 == y % 4) && (0 != y % 100) || (0 == y % 400)) ? 29 : 28);
	}
	else {
		d = day[m - 1];
	}
	return d;
}

int GetDaysInYear(int y) {
	return (((0 == y % 4) && (0 != y % 100) || (0 == y % 400)) ? 366 : 365);
}

int GetYesterday(int nDate) {
	int nYear = nDate / 10000;
	int nMonth = nDate / 100 % 100;
	int nday = nDate % 100;
	int nHour = 0;
	int nMin = 0;
	int nSecond = 0;
	struct tm ptm1;

	ptm1.tm_year = nYear - 1900;
	ptm1.tm_mon = nMonth - 1;
	ptm1.tm_mday = nday;
	ptm1.tm_hour = nHour;
	ptm1.tm_min = nMin;
	ptm1.tm_sec = nSecond;

	time_t lt = mktime(&ptm1);
	lt -= 24 * 60 * 60;
	struct tm* ptm2 = localtime(&lt);
	nYear = ptm2->tm_year + 1900;
	nMonth = ptm2->tm_mon + 1;
	nday = ptm2->tm_mday;
	return nYear * 10000 + nMonth * 100 + nday;
}


//传入数字，获取后一天
int GetTomorrow(int nDate) {
	int nYear = nDate / 10000;
	int nMonth = nDate / 100 % 100;
	int nday = nDate % 100;
	int nHour = 0;
	int nMin = 0;
	int nSecond = 0;
	struct tm ptm1;

	ptm1.tm_year = nYear - 1900;
	ptm1.tm_mon = nMonth - 1;
	ptm1.tm_mday = nday;
	ptm1.tm_hour = nHour;
	ptm1.tm_min = nMin;
	ptm1.tm_sec = nSecond;

	time_t lt = mktime(&ptm1);
	lt += 24 * 60 * 60;
	struct tm* ptm2 = localtime(&lt);
	nYear = ptm2->tm_year + 1900;
	nMonth = ptm2->tm_mon + 1;
	nday = ptm2->tm_mday;
	return nYear * 10000 + nMonth * 100 + nday;
}

void TraverseYear(int year) {
	int first_day = year * 10000 + 1 * 100 + 1;
	int days = GetDaysInYear(year);
	std::cout << "First day In " << year << "year is " << first_day << std::endl;
	std::cout << "There are " << days << " days In " << year << " year" << std::endl;
	std::cout << first_day / 10000 << "-" << (first_day % 10000) / 100 << "-" << first_day % 100 << std::endl;
	std::set<int> sDate;
	std::set<int> sWeek;
	int week = 0;
	for (int i = 0; i < days; ++i) {
		sDate.insert(first_day);
		week = GetWeek(first_day / 10000, (first_day % 10000) / 100, first_day % 100);
		if (0 == week || 6 == week) {
			sWeek.insert(first_day);
		}
		if (i % 10 == 0) {
			std::cout << first_day << "[" << week << "]" << std::endl;
		}
		else {
			std::cout << first_day << "[" << week << "]" << " ";
		}
		first_day = GetTomorrow(first_day);
	}
	std::cout << std::endl;

	std::string strFile = "";
	if (year == 2018) {
		sWeek.insert(20180101);		//元旦

		sWeek.insert(20180215);		//除夕
		sWeek.insert(20180216);		//春节
		sWeek.insert(20180217);
		sWeek.insert(20180218);
		sWeek.insert(20180219);
		sWeek.insert(20180220);
		sWeek.insert(20180221);		//春节结束

		sWeek.insert(20180405);		//清明节开始
		sWeek.insert(20180406);
		sWeek.insert(20180407);		//清明节结束

		sWeek.insert(20180429);		//劳动节开始	
		sWeek.insert(20180430);
		sWeek.insert(20180501);		//劳动节结束

		sWeek.insert(20180616);		//端午节开始
		sWeek.insert(20180617);
		sWeek.insert(20180618);		//端午节结束

		sWeek.insert(20180922);		//中秋节开始
		sWeek.insert(20180923);
		sWeek.insert(20180924);		//中秋节结束

		sWeek.insert(20181001);		//国庆节开始
		sWeek.insert(20181002);
		sWeek.insert(20181003);
		sWeek.insert(20181004);
		sWeek.insert(20181005);
		sWeek.insert(20181006);
		sWeek.insert(20181007);		//国庆节结束

		strFile = "2018.UnTradeDate";
	}
	else {
		sWeek.insert(20170101);		//元旦
		sWeek.insert(20170102);		//元旦


		sWeek.insert(20170127);		//除夕
		sWeek.insert(20170128);		//春节
		sWeek.insert(20170129);		//春节
		sWeek.insert(20170130);		//春节
		sWeek.insert(20170131);		//春节
		sWeek.insert(20170201);		//春节
		sWeek.insert(20170202);		//春节
		sWeek.insert(20170203);		//春节
		sWeek.insert(20170204);		//春节


		sWeek.insert(20170402);		//清明节
		sWeek.insert(20170403);		//清明节
		sWeek.insert(20170404);		//清明节


		sWeek.insert(20170429);		//劳动节
		sWeek.insert(20170430);		//劳动节
		sWeek.insert(20170501);		//劳动节


		sWeek.insert(20170528);		//端午节
		sWeek.insert(20170529);		//端午节
		sWeek.insert(20170530);		//端午节


		sWeek.insert(20171001);		//国庆、中秋
		sWeek.insert(20171002);		//国庆、中秋
		sWeek.insert(20171003);		//国庆、中秋
		sWeek.insert(20171004);		//国庆、中秋
		sWeek.insert(20171005);		//国庆、中秋
		sWeek.insert(20171006);		//国庆、中秋
		sWeek.insert(20171007);		//国庆、中秋
		sWeek.insert(20171008);		//国庆、中秋

		strFile = "2017.UnTradeDate";
	}

	std::cout << "------------weekend and vacation[" << sWeek.size() << "]------------" << std::endl;
	int i = 0;
	os::file fi(strFile.c_str(), "w+");
	for (std::set<int>::iterator it = sWeek.begin();
		it != sWeek.end(); ++it, ++i) {
		fi.write(str::to_string(*it));
		fi.write("\r\n");
		week = GetWeek(*it / 10000, (*it % 10000) / 100, *it % 100);
		if (i % 10 == 0) {
			std::cout << *it << "[" << week << "]" << std::endl;
		}
		else {
			std::cout << *it << "[" << week << "]" << " ";
		}
	}
	std::cout << std::endl;
}

static void testInherit();

static int odd_test();

#include "evhandler.h"
#include "server.h"
#include "../util/util.h"

static void sig_cb(int signo);


//allen::SyncEvent ev;
evhandler ev_handler;

void Srv() {
	int32 flow = 0;
	for (;; ++flow) {
		sys::timer t;
		//errorlog::err_msg("[Srv]==> step01");
		LOG << "[Srv]==> step01";
		/*errorlog::err_msg("UTC:	us[%ld]-ms[%ld]-sec[%ld]-hour[%ld]-day[%ld]",
			sys::xx::utc::us(), sys::xx::utc::ms(), sys::xx::utc::sec(),
			sys::xx::utc::hour(), sys::xx::utc::day());
		errorlog::err_msg("local_time: %s", sys::xx::local_time::to_string().c_str());*/
		/*TRY{
			int *s = 0;
			(*s) = 1;
		}END_TRY*/
		//sys::sleep(4);
		//ev.signal();
		//ev_handler.signal(flow);
		//errorlog::err_msg("[Srv]==> step02,time elapse: %ld ms", t.ms());
		LOG << "[Srv]==> step02,time elapse: " << t.ms() << " ms";
	}
}

void Cli() {
	int32 flow = 0;
	for (;; ++flow) {
		//errorlog::err_msg("[Cli]==> step01");
		LOG << "[Cli]==> step01";
		//ev_handler.install_ev_handler(flow);
		//if (!ev_handler.timed_wait(flow, 5000)){
		//	//errorlog::err_msg("[Cli]==> wait timeout");
		//	WLOG << "[Cli]==> wait timeout";
		//}
		//ev_handler.uninstall_ev_handler(flow);
		/*if (!ev.timed_wait(5000)){
			errorlog::err_msg("[Cli]==> wait timeout");
		}*/
		//errorlog::err_msg("[Cli]==> step02");
		LOG << "[Cli]==> step02";
	}
}

void test_env() {
	char *p;
	if (NULL != (p = getenv("USER")))
		errorlog::err_msg("USER = %s", p);
	//putenv("");
}

std::string enjsonlegal(const std::string &src) {
	std::string dst;
	for (std::string::const_iterator it = src.begin();
		it != src.end(); ++it) {
		switch (*it) {
		case '\"':
			dst += "\"";
			break;
			/*case ':':
				dst += "\:";
				break;*/
		default:
			dst += *it;
			break;
		}
	}
	return dst;
}

inline void set_green() {
	std::cout << "\033[0;32m"; \
		std::cout.flush();
}

inline void set_red() {
	std::cout << "\033[0;31m";
	std::cout.flush();
}

inline void set_lightblue() {
	std::cout << "\033[1;34m";
	std::cout.flush();
}

inline void reset_color() {
	std::cout << "\033[0m";//close all attributes
	std::cout.flush();
}

#define VERIFY_FLOAT_NAN(x) (((*((int32 *)&x)) & 0x7F800000) == 0x7F800000 && ((*((int32 *)&x)) & 0x7FFFFF) != 0)
#define VERIFY_DOUBLE_NAN(x) ((*((int64 *)&x) & 0x7FF0000000000000 == 0x7FF0000000000000) && (* ((int64 *)&x) & 0xfffffffffffff != 0))
#define VERIFY_DOUBLE(x) !(::fabs(x) <= 1e-7 || x >= DBL_MAX || x <= DBL_MIN || VERIFY_DOUBLE_NAN(x))



void test_color() {
	for (int i = 0; ;++i){
		if (i % 2){
			set_red();
			double value = 3.1415926456789049098654568987656789987654589876545678;
			std::cout << value << ": " << VERIFY_DOUBLE(value) << ": " << VERIFY_FLOAT_NAN(value) << std::endl;
			reset_color();
		}else {
			set_green();
			double value = 3.1415926456789345679098765456780;
			std::cout << value << ": " << VERIFY_DOUBLE(value) << ": " << VERIFY_FLOAT_NAN(value) << std::endl;
			reset_color();
		}
		sys::sleep(1);
	}
}

int main_backup(int argc, char** argv) {
	std::string str = "123456789012345678901234567890123456789\"";
	std::cout << str << ": " << enjsonlegal(str) << std::endl;

	test_color();
	/*LOG_INIT("log", "allen", rlog::DEBUG);
	allen::Thread thread(thread_fun);
	thread.start();
	thread.join();
	sleep(10);*/

	////建立信号处理机制
	//signal(SIGTERM,sig_cb);
	//signal(SIGINT,sig_cb);
	//
	//const std::string srvpath("/tmp/unixsocket");
	//selectsrv::server unix_srv(srvpath);
	//unix_srv.run();

	//cclog::close_cclog();

	/*pid_t pid = fork();
	if (pid > 0) {
		std::string log_dir = "./log/parent/";
		if (!os::path.exists(log_dir)) {
			os::makedirs(log_dir);
		}
		::FLG_log_dir = log_dir;
		cclog::init_cclog("launch");
		for (int i = 0; i < 100000; ++i) {
			LOG << "[parent]initialize server...";
			WLOG << "[parent]warn log...";
			ELOG << "[parent]error log...";
		}
	}
	else if (pid == 0) {
		std::string log_dir = "./log/child/";
		if (!os::path.exists(log_dir)) {
			os::makedirs(log_dir);
		}
		::FLG_log_dir = log_dir;
		cclog::init_cclog("launch");
		for (int i = 0; i < 100000; ++i) {
			LOG << "[child]initialize server...";
			WLOG << "[child]warn log...";
			ELOG << "[child]error log...";
		}
	}
	else {

	}

	sys::sleep(5);*/

	/*int flag = SA_RESTART | SA_ONSTACK;
	sys::signal.add_handler(SIGSEGV, &sig_cb, flag);
	sys::signal.add_handler(SIGABRT, &sig_cb, flag);
	sys::signal.add_handler(SIGFPE, &sig_cb, flag);*/



	//Debug::DeathHandler dh;

	/*allen::Thread threadS(Srv);
	threadS.start();

	allen::Thread threadC(Cli);
	threadC.start();

	threadS.join();
	threadC.join();*/


	//test_env();


	//std::string str("1234567890qwertyuiopasdfghjklasdfghjklzxcvbnm");
	////std::cout << "md5: " << util::md5(str) << std::endl;
	////std::cout << "md5: " << util::md5(str,true) << std::endl;

	//errorlog::err_msg("get current process name: %s", os::get_process_name().c_str());

	//os::chdir("/root/src/debug/bin/");
	//errorlog::err_msg("changes the current working directory of the calling process to the directory specified in /root/src/debug/bin/");
	//errorlog::err_msg("current working directory: %s:%s", os::getcwd().c_str(), os::get_current_dir_name().c_str());

	//os::system("/root/src/debug/bin/run.sh stop");
	//errorlog::err_msg("/root/src/debug/bin/run.sh stop");

	//os::system("/root/src/debug/bin/run.sh start");
	//errorlog::err_msg("/root/src/debug/bin/run.sh start");

	/*sys::signal.del_handler(SIGSEGV);
	sys::signal.del_handler(SIGABRT);
	sys::signal.del_handler(SIGFPE);*/

	return 0;
}


void sig_cb(int signo) {
	switch (signo) {
	case SIGHUP:
		fprintf(stderr, "catch SIGHUP signal,exit\n");
		break;
	case SIGINT:
		fprintf(stderr, "catch SIGINT signal,exit\n");
		exit(EXIT_SUCCESS);
		break;
	case SIGQUIT:
		fprintf(stderr, "catch SIGQUIT signal,exit\n");
		break;
	case SIGILL:
		fprintf(stderr, "catch SIGILL signal,exit\n");
		break;
	case SIGABRT:
		fprintf(stderr, "catch SIGABRT signal,exit\n");
		break;
	case SIGFPE:
		fprintf(stderr, "catch SIGFPE signal,exit\n");
		break;
	case SIGKILL:
		fprintf(stderr, "catch SIGKILL signal,exit\n");
		break;
	case SIGSEGV:
		fprintf(stderr, "catch SIGSEGV signal,exit\n");
		break;
	case SIGPIPE:
		fprintf(stderr, "catch SIGPIPE signal,exit\n");
		break;
	case SIGALRM:
		fprintf(stderr, "catch SIGALRM signal,exit\n");
		break;
	case SIGTERM:
		fprintf(stderr, "catch SIGTERM signal,exit\n");
		exit(EXIT_SUCCESS);
		break;
	case SIGUSR1:
		fprintf(stderr, "catch SIGUSR1 signal,exit\n");
		break;
	case SIGUSR2:
		fprintf(stderr, "catch SIGUSR2 signal,exit\n");
		break;
	case SIGCHLD:
		fprintf(stderr, "catch SIGCHLD signal,exit\n");
		break;
	case SIGCONT:
		fprintf(stderr, "catch SIGCONT signal,exit\n");
		break;
	case SIGSTOP:
		fprintf(stderr, "catch SIGSTOP signal,exit\n");
		break;
	case SIGTSTP:
		fprintf(stderr, "catch SIGTSTP signal,exit\n");
		break;
	case SIGTTIN:
		fprintf(stderr, "catch SIGTTIN signal,exit\n");
		break;
	case SIGTTOU:
		fprintf(stderr, "catch SIGTTOU signal,exit\n");
		break;
	default:
		fprintf(stderr, "catch unknow signal,exit\n");
		break;
	}

	return;

}

class SockBase {
public:
	SockBase(int fd) :_fd(fd) {}
	virtual ~SockBase() {}

	virtual int fdno() {
		return _fd;
	}
protected:
	int _fd;
};

class FSock :public SockBase {
public:
	FSock(int fd, int fid) :SockBase(fd), _fid(fid) {

	}
	virtual ~FSock() {}

	virtual int fdno() {
		return _fid;
	}

	int typeno() {
		return _fid * 2;
	}
private:
	int _fid;
};

class BSock :public SockBase {
public:
	BSock(int fd, int bid) :SockBase(fd), _bid(bid) {

	}
	virtual ~BSock() {}

	virtual int fdno() {
		return _bid;
	}

	int typeno() {
		return _bid * 3;
	}
private:
	int _bid;
};

void testInherit() {
	std::map<int, SockBase *> mBase;
	for (int i = 0; i < 10; ++i) {
		FSock *pFSock = new FSock(i, i * 10);
		mBase.insert(std::make_pair(i, pFSock));
	}

	for (std::map<int, SockBase *>::iterator it = mBase.begin();
		it != mBase.end(); ++it) {
		FSock *pFSock = (FSock *)it->second;
		std::cout << "fdno: " << it->second->fdno() << std::endl;
		std::cout << "typeno: " << pFSock->typeno() << std::endl;
	}
}


int odd_test() {
	ref_case3();
	case6();
	lexical_cast_test();
	test_function01();
	test_function03();

	char ch = 'x';
	std::string str("allen");
	str.append(2, ch);
	std::cout << str << std::endl;


	std::vector<std::string> vDst;
	std::string strSrc("x||y");
	str::split(strSrc, '|', vDst);
	std::vector<std::string>::iterator it = vDst.begin();
	for (; it != vDst.end(); ++it)
	{
		std::cout << *it << "_";
	}
	std::cout << std::endl;

	strSrc = "   xxx   ";
	std::cout << str::strip(strSrc) << std::endl;


	char test01[] = "     yyyyy";
	str::lstrip_nc(test01);
	std::cout << "[" << test01 << "]" << std::endl;

	char test02[] = "abcd			";
	str::rstrip_nc(test02);
	std::cout << "[" << test02 << "]" << std::endl;


	char test03[] = "		oooo			";
	str::strip_nc(test03);
	std::cout << "[" << test03 << "]" << std::endl;

	std::cout << str::get_ab_pinyin("邓璐强") << std::endl;
	std::string strFullPY = "";
	str::get_full_pinyin((unsigned char *)"邓璐强", strFullPY, true);
	std::cout << strFullPY << std::endl;

	std::string strtest04 = "ab";
	ch = '\0';
	std::string strDst = "";
	std::string strDefault = "";
	//strtest04 += util::safe_lexical_cast(ch,strDst,strDefault);
	strtest04.append(1, ch);
	strtest04 += "cd";
	std::cout << "[" << strtest04 << "]" << std::endl;

	char szData[20] = { 0 };
	snprintf(szData, 19, "%0.6f", 12.45678909876545678);
	printf("[%s]\n", szData);


	CRC16 crc16(CRC16::eCCITT_FALSE);
	char data1[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	char data2[] = { '5', '6', '7', '8', '9' };
	unsigned short c1, c2;
	c1 = crc16.crcCompute(data1, 9);
	c2 = crc16.crcCompute(data1, 4, true);
	c2 = crc16.crcCompute(data2, 5, false);


	printf("%04x\n", c1);
	printf("%04x\n", c2);

	int* param1 = NULL;
	CHECK_PARAMS(param1, -1);
	test_function();

	std::string strRet("abcd");
	strRet += CHECK_CHAR('x');
	strRet += "efgh";
	std::cout << "[" << strRet << "]" << std::endl;


	ch = 'd';
	strRet += ch;
	std::cout << "[" << strRet << "]" << std::endl;


	StreamBuf sb;
	sb << "hello " << 3
		<< " " << true
		<< " " << false
		<< " " << 3.14
		<< " " << &sb;

	std::cout << sb.to_string() << std::endl;
	sb.clear();

	std::string s(8192, 'x');
	sb << s;
	std::cout << sb.size() << " " << s.size() << std::endl;

	//%f  表示按浮点数的格式输出
	//%e  表示按指数形式的浮点数的格式输出
	//%g  表示自动选择合适的表示法输出(根据数值不同自动选择 %f 或 %e ，%e 格式在指数小于-4或者大于等于精度时使用)
	std::cout << "3.39999999999 --> " << str::to_string(3.39999999999) << std::endl;//3.4
	std::cout << "3.3999999999 --> " << str::to_string(3.3999999999) << std::endl;//3.4
	std::cout << "3.399999999 --> " << str::to_string(3.399999999) << std::endl;//3.4
	std::cout << "3.39999999 --> " << str::to_string(3.39999999) << std::endl;//3.4
	std::cout << "3.3999999 --> " << str::to_string(3.3999999) << std::endl;//3.4
	std::cout << "3.399999 --> " << str::to_string(3.399999) << std::endl;
	std::cout << "3.39999 --> " << str::to_string(3.39999) << std::endl;
	std::cout << "3.3999 --> " << str::to_string(3.3999) << std::endl;
	std::cout << "3.399 --> " << str::to_string(3.399) << std::endl;
	std::cout << "3.39 --> " << str::to_string(3.39) << std::endl;

	printf("--------------------------\n");
	double dtest = 3.3999999999;
	printf("%.16lg | %lf\n", dtest, dtest);


	/*std::string strRet;
	os::file fi("te.txt", "r+");
	std::vector<std::string> v = fi.getlines();
	if (v.size() == 2){
	std::string str = str::strip(v[1]);
	strRet = "{\"error_no\":\"0\",\"results\":[{\"title\":\"";
	strRet += v[0];
	strRet += "\",\"content\":\"";
	strRet += str;
	strRet += "\"}],\"dsName\":[\"results\"],\"error_info\":\"Risk Warn\"}";
	}else{
	strRet = "";
	}*/

	os::makedirs("_os.mkdirs/x/y/z");
	os::makedirs("_os.single/x");

	if (os::xx::path::exists("_os.single/x")) {
		printf("(1)exist\n");
	}
	else {
		printf("(1)doesn't exist\n");
	}

	if (!os::rmdir("_os.single/x")) {
		printf("rmdir failed\n");
	}

	if (os::xx::path::exists("_os.single/x")) {
		printf("(2)exist\n");
	}
	else {
		printf("(2)doesn't exist\n");
	}

	if (!os::rmdirs("_os.single/x")) {
		printf("rmdirs failed\n");
	}

	if (os::xx::path::exists("_os.single/x")) {
		printf("(3)exist\n");
	}
	else {
		printf("(3)doesn't exist\n");
	}

	if (os::xx::path::exists("RiskWarn")) {
		printf("(4)exist\n");
	}
	else {
		printf("(4)doesn't exist\n");
	}

	//首先遍历所有日期，去除周末，加入set容器，然后再遍历所有节假日，加入set容器
	std::cout << "current year: " << GetCurYear() << std::endl;
	std::cout << "current month: " << GetCurMonth() << std::endl;
	std::cout << "current day: " << GetCurDay() << std::endl;
	std::cout << "current week: " << GetCurWeek() << std::endl;

	std::cout << "week: " << GetWeek(GetCurYear(), GetCurMonth(), GetCurDay()) << std::endl;
	std::cout << "GetWeek(2017, 12, 13)-week: " << GetWeek(2017, 12, 13) << std::endl;
	std::cout << "GetWeek(2018, 1, 3)-week: " << GetWeek(2017, 12, 3) << std::endl;
	std::cout << "GetWeek(2018, 1, 10)-week: " << GetWeek(2017, 12, 10) << std::endl;


	std::cout << GetDaysInMonth(2018, 1) << std::endl;
	std::cout << GetDaysInMonth(2018, 2) << std::endl;
	std::cout << GetDaysInMonth(2017, 12) << std::endl;
	std::cout << GetDaysInMonth(2017, 11) << std::endl;

	TraverseYear(2018);
	TraverseYear(2017);
}