#include "../src/server.cpp"
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
  std::thread server_thread;

  void SetUp() override {
    server_thread = std::thread([]() {
      HttpServer server(8081);
      server.start();
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  void TearDown() override {
    // Note: In a real implementation, we'd need a way to gracefully stop the
    // server
    server_thread.detach();
  }

  std::string makeRequest(const std::string &url,
                          const std::string &method = "GET",
                          const std::string &data = "") {
    CURL *curl = curl_easy_init();
    std::string response_string;

    if (curl) {
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

      if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
      }

      curl_easy_perform(curl);
      curl_easy_cleanup(curl);
    }

    return response_string;
  }
};

TEST_F(ServerTest, TestHelloEndpoint) {
  std::string response = makeRequest("http://localhost:8081/api/hello");
  json response_json = json::parse(response);

  EXPECT_EQ(response_json["status"], "success");
  EXPECT_EQ(response_json["message"], "Hello, World!");
}

TEST_F(ServerTest, TestEchoEndpoint) {
  json test_data = {{"test", "data"}};
  std::string response =
      makeRequest("http://localhost:8081/api/echo", "POST", test_data.dump());
  json response_json = json::parse(response);

  EXPECT_EQ(response_json["status"], "success");
  EXPECT_EQ(response_json["echo"], test_data);
}

TEST_F(ServerTest, TestNotFound) {
  std::string response = makeRequest("http://localhost:8081/api/nonexistent");
  json response_json = json::parse(response);

  EXPECT_EQ(response_json["status"], "error");
  EXPECT_EQ(response_json["error"], "Not Found");
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
