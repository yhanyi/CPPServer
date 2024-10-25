CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -pthread -I. -I/usr/include/nlohmann
CXXFLAGS += -I/opt/homebrew/include -I/usr/local/include
LDFLAGS = -pthread -L/opt/homebrew/lib -L/usr/local/lib

# Add PostgreSQL flags
PG_INCLUDE = -I/opt/homebrew/include
PG_LIBS = -L/opt/homebrew/lib -lpqxx

# Prometheus settings
PROMETHEUS_LIBS = -lprometheus-cpp-core -lprometheus-cpp-pull -lz

# Test settings
TEST_LIBS = -lgtest -lgtest_main
SERVER_TEST_LIBS = $(TEST_LIBS) -lcurl

# Output directories
BUILD_DIR = build
TEST_DIR = $(BUILD_DIR)/tests

# Create directories
$(shell mkdir -p $(BUILD_DIR) $(TEST_DIR))

# Main server target
server: src/server.cpp
	$(CXX) $(CXXFLAGS) $(PG_INCLUDE) -DMAIN $< -o $@ $(LDFLAGS) $(PG_LIBS) $(PROMETHEUS_LIBS)

# Test targets
all: server $(TEST_DIR)/server_tests $(TEST_DIR)/cache_tests

$(TEST_DIR)/server_tests: tests/server_tests.cpp src/server.cpp
	$(CXX) $(CXXFLAGS) $(PG_INCLUDE) $< -o $@ $(LDFLAGS) $(PG_LIBS) $(PROMETHEUS_LIBS) $(SERVER_TEST_LIBS)

$(TEST_DIR)/cache_tests: tests/cache_tests.cpp src/cache.hpp
	$(CXX) $(CXXFLAGS) $(PG_INCLUDE) $< -o $@ $(LDFLAGS) $(PG_LIBS) $(PROMETHEUS_LIBS) $(TEST_LIBS)

test: $(TEST_DIR)/server_tests $(TEST_DIR)/cache_tests
	@echo "Running tests..."
	@$(TEST_DIR)/cache_tests --gtest_output="xml:$(TEST_DIR)/cache_tests.xml"
	@$(TEST_DIR)/server_tests --gtest_output="xml:$(TEST_DIR)/server_tests.xml"

clean:
	rm -rf $(BUILD_DIR)
	rm -f server

.PHONY: all clean test server
