#include "../src/server.cpp"
#include <atomic>
#include <chrono>
#include <curl/curl.h>
#include <future>
#include <gtest/gtest.h>
#include <thread>

size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                     std::string *userp) {
  userp->append((char *)contents, size * nmemb);
  return size * nmemb;
}

class ServerTest : public ::testing::Test {
protected:
  std::unique_ptr<std::thread> server_thread;
  std::shared_ptr<std::atomic<bool>> stop_signal;
  int port;
  const int TEST_TIMEOUT = 10;

  void SetUp() override {
    port = 10000 + (rand() % 55535);
    stop_signal = std::make_shared<std::atomic<bool>>(false);

    server_thread = std::make_unique<std::thread>([this]() {
      HttpServer server(port, *stop_signal);
      server.start();
    });

    // Wait for server to start with timeout
    auto start_time = std::chrono::steady_clock::now();
    bool server_ready = false;
    while (!server_ready && std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::steady_clock::now() - start_time)
                                    .count() < TEST_TIMEOUT) {
      CURL *curl = curl_easy_init();
      if (curl) {
        std::string url =
            "http://localhost:" + std::to_string(port) + "/api/hello";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res == CURLE_OK) {
          server_ready = true;
          break;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ASSERT_TRUE(server_ready) << "Server failed to start within timeout";
  }

  void TearDown() override {
    if (server_thread && server_thread->joinable()) {
      stop_signal->store(true);
      server_thread->join();
    }
    // Add longer delay between tests
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  std::string makeRequest(const std::string &endpoint,
                          const std::string &method = "GET",
                          const std::string &data = "") {
    std::string response_string;
    CURL *curl = curl_easy_init();

    if (curl) {
      std::string url = "http://localhost:" + std::to_string(port) + endpoint;
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

      struct curl_slist *headers = NULL;
      headers = curl_slist_append(headers, "Content-Type: application/json");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

      if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
      }

      CURLcode res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
                  << std::endl;
      }

      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);
    }

    return response_string;
  }
};

TEST_F(ServerTest, TestHelloEndpoint) {
  std::string response = makeRequest("/api/hello");
  json response_json = json::parse(response);

  EXPECT_EQ(response_json["status"], "success");
  EXPECT_EQ(response_json["message"], "Hello, World!");
}

TEST_F(ServerTest, TestEchoEndpoint) {
  json test_data = {{"test", "data"}};
  std::string response = makeRequest("/api/echo", "POST", test_data.dump());
  json response_json = json::parse(response);

  EXPECT_EQ(response_json["status"], "success");
  EXPECT_EQ(response_json["echo"], test_data);
}

TEST_F(ServerTest, TestCacheAddEntry) {
  json test_data = {{"key", "test_key"}, {"value", "test_value"}, {"ttl", 60}};
  std::string response = makeRequest("/api/cached", "POST", test_data.dump());
  json response_json = json::parse(response);

  EXPECT_EQ(response_json["status"], "success");
  EXPECT_EQ(response_json["key"], "test_key");
  EXPECT_EQ(response_json["ttl"], 60);
}

TEST_F(ServerTest, TestCacheRetrieveEntry) {
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Add entry
  json test_data = {{"key", "test_key"}, {"value", "test_value"}, {"ttl", 60}};

  // Print request details for debugging
  std::cout << "Sending POST to port " << port << std::endl;
  std::string add_response =
      makeRequest("/api/cached", "POST", test_data.dump());
  std::cout << "POST response: " << add_response << std::endl;

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Get entry with proper HTTP headers
  std::cout << "Sending GET to port " << port << std::endl;
  std::string get_response = makeRequest("/api/cached/test_key");
  std::cout << "GET response: " << get_response << std::endl;

  try {
    json response_json = json::parse(get_response);
    EXPECT_EQ(response_json["status"], "success");
    EXPECT_EQ(response_json["key"], "test_key");
    EXPECT_EQ(response_json["value"], "test_value");
  } catch (const json::parse_error &e) {
    FAIL() << "Invalid JSON response: " << e.what()
           << "\nResponse was: " << get_response;
  }
}

TEST_F(ServerTest, TestCacheKeyNotFound) {
  // Add wait to ensure server is ready
  std::this_thread::sleep_for(std::chrono::seconds(1));

  try {
    std::string response = makeRequest("/api/cached/nonexistent_key", "GET");
    std::cout << "Raw response: " << response << std::endl;

    if (response.empty()) {
      FAIL() << "Empty response received";
    }

    json response_json = json::parse(response);
    EXPECT_EQ(response_json["status"], "error");
    EXPECT_EQ(response_json["error"], "Key not found");
  } catch (const json::parse_error &e) {
    FAIL() << "JSON parse error: " << e.what();
  } catch (const std::exception &e) {
    FAIL() << "Other error: " << e.what();
  }
}

TEST_F(ServerTest, TestCacheExpiry) {
  json test_data = {
      {"key", "expiring_key"}, {"value", "test_value"}, {"ttl", 1}
      // 1 second TTL
  };
  makeRequest("/api/cached", "POST", test_data.dump());

  std::this_thread::sleep_for(std::chrono::seconds(2));

  std::string response = makeRequest("/api/cached/expiring_key");
  json response_json = json::parse(response);

  EXPECT_EQ(response_json["status"], "error");
  EXPECT_EQ(response_json["error"], "Key not found");
}

TEST_F(ServerTest, TestCacheClear) {
  // Add entry
  json test_data = {{"key", "test_key"}, {"value", "test_value"}};
  makeRequest("/api/cached", "POST", test_data.dump());

  // Clear cache
  std::string clear_response = makeRequest("/api/cache/clear", "POST");
  json clear_json = json::parse(clear_response);
  EXPECT_EQ(clear_json["status"], "success");

  // Try to retrieve cleared entry
  std::string get_response = makeRequest("/api/cached/test_key");
  json get_json = json::parse(get_response);
  EXPECT_EQ(get_json["status"], "error");
}

TEST_F(ServerTest, TestInvalidJSON) {
  std::string invalid_json = "{invalid_json}";
  std::string response = makeRequest("/api/cached", "POST", invalid_json);
  json response_json = json::parse(response);

  EXPECT_EQ(response_json["status"], "error");
  EXPECT_EQ(response_json["error"], "Invalid JSON");
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  int result = RUN_ALL_TESTS();
  curl_global_cleanup();
  return result;
}
