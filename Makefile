CXX = g++
CXXFLAGS = -std=c++17 -I.
LDFLAGS = -pthread

all: server tests

server: src/server.cpp
	$(CXX) $(CXXFLAGS) -DMAIN src/server.cpp -o server $(LDFLAGS)

tests: tests/server_tests.cpp
	$(CXX) $(CXXFLAGS) tests/server_tests.cpp -o server_tests $(LDFLAGS) -lgtest -lcurl

clean:
	rm -f server server_tests

.PHONY: all clean tests
