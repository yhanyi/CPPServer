#include "../src/server.cpp"
#include <atomic>
#include <curl/curl.h>
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

  void SetUp() override {
    static int next_port = 8081;
    port = next_port++;
    stop_signal = std::make_shared<std::atomic<bool>>(false);
    server_thread = std::make_unique<std::thread>([this]() {
      try {
        HttpServer server(port, *stop_signal);
        server.start();
      } catch (const std::exception &e) {
        std::cerr << "Server error: " << e.what() << std::endl;
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  void TearDown() override {
    stop_signal->store(true);
    if (server_thread && server_thread->joinable()) {
      server_thread->join();
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
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

// TEST_F(ServerTest, TestCachedEndpoint) {
//     std::string response1 = makeRequest("/api/cached");
//     json response_json1 = json::parse(response1);
//     EXPECT_FALSE(response_json1["cached"]);
//
//     std::string response2 = makeRequest("/api/cached");
//     json response_json2 = json::parse(response2);
//     EXPECT_EQ(response2, response1);
// }

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  int result = RUN_ALL_TESTS();
  curl_global_cleanup();
  return result;
}
