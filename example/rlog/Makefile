#子目录的Makefile直接读取其子目录就行
SUBDIRS=$(shell ls -l | grep ^d | awk '{print $$9}')

#C编译参数
CFLAGS := -Wall -DOPENSSL_THREADS  -D_THREAD_SAFE -D_THREADSAFE -D_REENTRANT -D_GNU_SOURCE -g -fPIC

#CPP编译参数
#CPPFLAGS := -Wall -O2 -g -std=c++11
CPPFLAGS := -Wall -O2 -g 

#获取当前目录下的c文件集，放在变量CUR_SOURCE中
CUR_SOURCE := $(wildcard *.c) $(wildcard *.cc)

#将对应的c文件名转为o文件后放在下面的CUR_OBJS变量中
CUR_OBJS := $(patsubst %.c, %.o, $(patsubst %.cc, %.o, $(CUR_SOURCE)))

all:$(SUBDIRS) $(CUR_OBJS)

$(SUBDIRS):ECHO
	make -C $@

#将c文件编译为o文件，并放在指定放置目标文件的目录中即OBJS_DIR
$(CUR_OBJS):%.o:%.cc
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@
	
ECHO:
	@echo $(SUBDIRS)
