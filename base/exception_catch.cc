#include "exception_catch.h"

namespace exception_catch {
	Except_frame* except_stack = new Except_frame;

	void error_dump() {
		const int maxLevel = 200;
		void* buffer[maxLevel];
		int level = ::backtrace(buffer, maxLevel);
		char cmd[1024] = "addr2line -C -f -e ";
		char* prog = cmd + strlen(cmd);
		::readlink("/proc/self/exe", prog, sizeof(cmd) - (prog - cmd) - 1);
		printf("cmd: [%s]\n", cmd);//就是当前程序运行的全路径以及程序名
		FILE* fp = ::popen(cmd, "w");
		if (!fp) {
			perror("popen");
			return;
		}

		printf("backtrace() returned %d addresses\n", level);
		char **strings = ::backtrace_symbols(buffer, level);
		if (strings == NULL) {
			perror("backtrace_symbols");
			exit(EXIT_FAILURE);
		}

		for (int j = 0; j < level; j++) 
			printf("  [%02d] %s\n", j, strings[j]);

		
		for (int i = 0; i < level; ++i) 
			fprintf(fp, "%p\n", buffer[i]);

		fclose(fp);
		free(strings);
	}

	void signal_cb(int sig) {
		switch (sig){
		case SIGSEGV:
			printf("received signal SIGSEGV\n");
			break;
		default:
			printf("received signal %d\n", sig);
			break;
		}
		error_dump();
		::siglongjmp(except_stack->env, 1);
	}
}