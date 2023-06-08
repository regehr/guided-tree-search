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

afl-sharedmem.o:	$(AFL)/src/afl-sharedmem.c
	$(CC) -I$(AFL)/include -g -O3 $(CPPFLAGS) -DBIN_PATH=\"dummy\" -Wno-pointer-sign -fPIC -c -o ./afl-sharedmem.o $(AFL)/src/afl-sharedmem.c

guide-gen.so:	afl-sharedmem.o afl-fuzz-queue.o afl-common.o guide-gen.cpp ../mutate/mutate.cpp
	$(CXX) -Wno-deprecated -g -O3 $(CXXFLAGS) $(CPPFLAGS) -shared -fPIC -o guide-gen.so -I$(AFL)/include -I../include -I../mutate -I../guided-tree-search/tests  guide-gen.cpp ../mutate/mutate.cpp ./afl-fuzz-queue.o $(AFL)/src/afl-performance.o ./afl-common.o ./afl-sharedmem.o

clean:
	rm -f guide-gen.so *.o *~ core
