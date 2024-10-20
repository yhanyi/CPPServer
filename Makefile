CXX = g++
CXXFLAGS = -std=c++17 -I. -I/usr/include/nlohmann -I/opt/homebrew/include
LDFLAGS = -pthread

# Prometheus flags - adjust paths based on your installation
PROMETHEUS_INCLUDE = -I/usr/local/include
PROMETHEUS_LIBS = -lprometheus-cpp-core -lprometheus-cpp-pull -lz

all: server tests cache_tests

server: src/server.cpp
	$(CXX) $(CXXFLAGS) $(PROMETHEUS_INCLUDE) -DMAIN src/server.cpp -o server $(LDFLAGS) $(PROMETHEUS_LIBS)

tests: tests/server_tests.cpp
	$(CXX) $(CXXFLAGS) $(PROMETHEUS_INCLUDE) tests/server_tests.cpp -o server_tests $(LDFLAGS) $(PROMETHEUS_LIBS) -lgtest -lgtest_main -lcurl

cache_tests: tests/cache_tests.cpp src/cache.hpp
	$(CXX) $(CXXFLAGS) $(PROMETHEUS_INCLUDE) tests/cache_tests.cpp -o cache_tests $(LDFLAGS) $(PROMETHEUS_LIBS) -lgtest -lgtest_main

prometheus_deps:
	@echo "Checking Prometheus dependencies..."
ifeq ($(shell uname), Darwin)
	@which brew > /dev/null || (echo "Installing Homebrew..." && /bin/bash -c "$$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)")
	@brew list prometheus-cpp > /dev/null || (echo "Installing prometheus-cpp..." && brew install prometheus-cpp)
	@brew list prometheus > /dev/null || (echo "Installing prometheus..." && brew install prometheus)
else
	@which apt-get > /dev/null && ( \
		sudo apt-get update && \
		sudo apt-get install -y libprometheus-cpp-dev prometheus \
	) || echo "Please install prometheus-cpp and prometheus manually"
endif

clean:
	rm -f server server_tests cache_tests

.PHONY: all clean tests cache_tests prometheus_deps
