#ifndef __EXCEPTION_H
#define __EXCEPTION_H

#include <setjmp.h>  
#include <stdlib.h>  
#include <stdarg.h>  
#include <execinfo.h>  
#include <stdio.h>  
#include <signal.h>  
#include <iostream>  
#include <string.h>  


namespace exception_catch {
	typedef struct Except_frame {
		Except_frame() {
			clear();
		}

		void clear(){
			flag = 0;
			::bzero(env, sizeof(env));
		}

		bool isDef(){
			return flag;
		}

		jmp_buf env;
		int flag;
	}Except_frame;


	extern Except_frame* except_stack;
	extern void error_dump();
	extern void signal_cb(int sig);
}


#define TRY exception_catch::except_stack->flag = sigsetjmp(exception_catch::except_stack->env, 1); \
if (!exception_catch::except_stack->isDef()) { \
	signal(SIGSEGV, exception_catch::signal_cb); \
	printf("start use TRY\n");


#define END_TRY } else {\
		exception_catch::except_stack->clear(); \
	}\
	printf("stop use TRY\n");


#define RETURN_NULL } else { \
		exception_catch::except_stack->clear(); \
	}\
	return NULL;


#define RETURN_PARAM  { exception_catch::except_stack->clear(); }\
	return x;


#define EXIT_ZERO } else { exception_catch::except_stack->clear(); }\
	exit(0);

#endif