all:
	$(CXX) -Wall -std=c++17 -O3 tester.cpp -o tester

debug:
	$(CXX) -g -Wall -std=c++17 tester.cpp -fsanitize=address,undefined -D_DEBUG -o tester

clean:
	rm -f tester
