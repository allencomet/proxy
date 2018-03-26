LIBS=-lm
OBJS=main.o error.o
CCFLAGS=-Wall -O2 -g
CC=g++
TARGETS=unixcli
$(TARGETS):$(OBJS)
	$(CC) -o unixcli $(OBJS) $(LIBS) $(CCFLAGS) -g
main.o:main.cc
	$(CC) -c main.cc -g
error.o:error.cc error.h
	$(CC) -c error.cc -g
.PHONY=clean
clean:
	rm -f $(TARGETS) $(OBJS)
