CXX = g++
CXXFLAGS = -std=c++17 -I. -I/usr/include/nlohmann -I/opt/homebrew/include
LDFLAGS = -pthread -L/opt/homebrew/lib

PROMETHEUS_INCLUDE = -I/usr/local/include -I/opt/homebrew/include
PROMETHEUS_LIBS = -lprometheus-cpp-core -lprometheus-cpp-pull -lz

# Add PostgreSQL flags
PG_INCLUDE = -I/opt/homebrew/include
PG_LIBS = -L/opt/homebrew/lib -lpqxx

ifeq ($(shell uname), Darwin)
    PROMETHEUS_INCLUDE += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib
else
    PROMETHEUS_INCLUDE += -I/usr/local/include
    LDFLAGS += -L/usr/local/lib
endif

all: server_tests cache_tests database_tests

server: src/server.cpp
	$(CXX) $(CXXFLAGS) $(PROMETHEUS_INCLUDE) $(PG_INCLUDE) -DMAIN $< -o $@ $(LDFLAGS) $(PROMETHEUS_LIBS) $(PG_LIBS)

database_tests: tests/database_tests.cpp
	$(CXX) $(CXXFLAGS) $(PROMETHEUS_INCLUDE) $(PG_INCLUDE) $< -o $@ $(LDFLAGS) $(PROMETHEUS_LIBS) $(PG_LIBS) -lgtest -lgtest_main

server_tests: tests/server_tests.cpp
	$(CXX) $(CXXFLAGS) $(PROMETHEUS_INCLUDE) $(PG_INCLUDE) $< -o $@ $(LDFLAGS) $(PROMETHEUS_LIBS) $(PG_LIBS) -lgtest -lgtest_main -lcurl

cache_tests: tests/cache_tests.cpp
	$(CXX) $(CXXFLAGS) $(PROMETHEUS_INCLUDE) $(PG_INCLUDE) $< -o $@ $(LDFLAGS) $(PROMETHEUS_LIBS) $(PG_LIBS) -lgtest -lgtest_main

prometheus_deps:
	@echo "Checking Prometheus dependencies..."
ifeq ($(shell uname), Darwin)
	@which brew > /dev/null || (echo "Installing Homebrew..." && /bin/bash -c "$$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)")
	@brew list prometheus-cpp > /dev/null || (echo "Installing prometheus-cpp..." && brew install prometheus-cpp)
	@brew list prometheus > /dev/null || (echo "Installing prometheus..." && brew install prometheus)
	@brew list postgresql@14 > /dev/null || (echo "Installing postgresql..." && brew install postgresql@14)
	@brew list libpqxx > /dev/null || (echo "Installing libpqxx..." && brew install libpqxx)
else
	@which apt-get > /dev/null && ( \
		sudo apt-get update && \
		sudo apt-get install -y libprometheus-cpp-dev prometheus postgresql libpqxx-dev \
	) || echo "Please install dependencies manually"
endif

clean:
	rm -f server server_tests cache_tests database_tests
	rm -rf data

.PHONY: all clean tests cache_tests prometheus_deps
