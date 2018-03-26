#pragma once



namespace errorlog{
extern int		daemon_proc;		/* set nonzero by daemon_init() */

/* Nonfatal error related to system call
 * Print message and return */
//va_start，函数名称，读取可变参数的过程其实就是在堆栈中，使用指针,遍历堆栈段中的参数列表,从低地址到高地址一个一个地把参数内容读出来的过程
//宏va_arg()、va_start()和va_end()一起使用，便可以完成向函数传入数目可变的变元操作。取可变数目变元的典型例子是函数printf()。类型va_list是在<stdarg.h>中定义的。
//
void err_ret(const char *fmt, ...);

/* Fatal error related to system call
 * Print message and terminate */
//fatal:致命的
void err_sys(const char *fmt, ...);

/* Fatal error related to system call
 * Print message, dump core, and terminate */
void err_dump(const char *fmt, ...);

/* Nonfatal error unrelated to system call
 * Print message and return */
void err_msg(const char *fmt, ...);

/* Fatal error unrelated to system call
 * Print message and terminate */
void err_quit(const char *fmt, ...);
}




