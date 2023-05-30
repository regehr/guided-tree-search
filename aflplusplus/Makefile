ifdef debug
	CPPLAGS += -fsanitize=address
	CXXFLAGS += -Wall
	CC := clang
	CXX := clang++
endif
ifdef DEBUG
	CPPFLAGS += -fsanitize=address
	CXXFLAGS += -Wall
	CC := clang
	CXX := clang++
endif

CXXFLAGS += -std=c++2a

all:	guide-gen.so

afl-fuzz-queue.o:	$(AFL)/src/afl-fuzz-queue.c
	$(CC) -D_STANDALONE_MODULE=1 -I$(AFL)/include -g -O3 $(CPPFLAGS) -fPIC -c -o ./afl-fuzz-queue.o $(AFL)/src/afl-fuzz-queue.c

afl-common.o:	$(AFL)/src/afl-common.c
	$(CC) -I$(AFL)/include -g -O3 $(CPPFLAGS) -DBIN_PATH=\"dummy\" -Wno-pointer-sign -fPIC -c -o ./afl-common.o $(AFL)/src/afl-common.c

guide-gen.so:	afl-fuzz-queue.o afl-common.o guide-gen.cpp mutate.cpp
	$(CXX) -Wno-deprecated -g -O3 $(CXXFLAGS) $(CPPFLAGS) -shared -fPIC -o guide-gen.so -I$(AFL)/include -I../include -I../guided-tree-search/tests  guide-gen.cpp mutate.cpp ./afl-fuzz-queue.o $(AFL)/src/afl-performance.o ./afl-common.o

clean:
	rm -f guide-gen.so *.o *~ core
