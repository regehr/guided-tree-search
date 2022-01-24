all:
	clang++ -Wall -std=c++20 -O3 tester.cpp -o tester

debug:
	clang++ -g -Wall -std=c++20 tester.cpp -fsanitize=address,undefined -D_DEBUG -o tester

clean:
	rm -f tester
