CXX = g++
CXXFLAGS = -std=c++17 -I. -I/usr/include/nlohmann -I/opt/homebrew/include -Isrc
LDFLAGS = -pthread -L/opt/homebrew/lib

PROMETHEUS_INCLUDE = -I/usr/local/include -I/opt/homebrew/include
PROMETHEUS_LIBS = -lprometheus-cpp-core -lprometheus-cpp-pull -lz

PG_INCLUDE = -I/opt/homebrew/include
PG_LIBS = -L/opt/homebrew/lib -lpqxx

ifeq ($(shell uname), Darwin)
    PROMETHEUS_INCLUDE += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib
else
    PROMETHEUS_INCLUDE += -I/usr/local/include
    LDFLAGS += -L/usr/local/lib
endif

SERVER_SRCS = src/server.cpp src/main.cpp
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)

all: server server_tests cache_tests

server: $(SERVER_OBJS)
	$(CXX) $(SERVER_OBJS) -o $@ $(LDFLAGS) $(PROMETHEUS_LIBS) $(PG_LIBS) -lcurl

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(PROMETHEUS_INCLUDE) $(PG_INCLUDE) -c $< -o $@

server_tests: tests/server_tests.cpp src/server.cpp
	$(CXX) $(CXXFLAGS) $(PROMETHEUS_INCLUDE) $(PG_INCLUDE) src/server.cpp tests/server_tests.cpp -o $@ $(LDFLAGS) $(PROMETHEUS_LIBS) $(PG_LIBS) -lgtest -lgtest_main -lcurl

cache_tests: tests/cache_tests.cpp
	$(CXX) $(CXXFLAGS) $(PROMETHEUS_INCLUDE) $(PG_INCLUDE) $< -o $@ $(LDFLAGS) $(PROMETHEUS_LIBS) $(PG_LIBS) -lgtest -lgtest_main

clean:
	rm -f server server_tests cache_tests $(SERVER_OBJS)
	rm -rf data

.PHONY: all clean tests cache_tests
