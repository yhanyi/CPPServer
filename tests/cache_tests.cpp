#include "../src/cache.hpp"
#include <future>
#include <gtest/gtest.h>
#include <thread>

class LRUCacheTest : public ::testing::Test {
protected:
  std::unique_ptr<LRUCache<std::string, std::string>> cache;

  void SetUp() override {
    cache = std::make_unique<LRUCache<std::string, std::string>>(
        3, std::chrono::seconds(5));
  }

  void TearDown() override { cache.reset(); }
};

TEST_F(LRUCacheTest, BasicPutGet) {
  cache->put("key1", "value1");
  std::string result;
  EXPECT_TRUE(cache->get("key1", result));
  EXPECT_EQ(result, "value1");
}

TEST_F(LRUCacheTest, CacheEviction) {
  cache->put("key1", "value1");
  cache->put("key2", "value2");
  cache->put("key3", "value3");
  cache->put("key4", "value4"); // Should evict key1

  std::string result;
  EXPECT_FALSE(cache->get("key1", result));
  EXPECT_TRUE(cache->get("key4", result));
}

TEST_F(LRUCacheTest, TTLExpiration) {
  cache->put("key1", "value1", std::chrono::seconds(1));
  std::string result;
  EXPECT_TRUE(cache->get("key1", result));

  std::this_thread::sleep_for(std::chrono::seconds(2));
  EXPECT_FALSE(cache->get("key1", result));
}

TEST_F(LRUCacheTest, ThreadSafety) {
  auto writer = std::async(std::launch::async, [this]() {
    for (int i = 0; i < 100; i++) {
      cache->put("key" + std::to_string(i), "value" + std::to_string(i));
    }
  });

  auto reader = std::async(std::launch::async, [this]() {
    std::string result;
    for (int i = 0; i < 100; i++) {
      cache->get("key" + std::to_string(i), result);
    }
  });

  writer.wait();
  reader.wait();
}

TEST_F(LRUCacheTest, CacheClear) {
  cache->put("key1", "value1");
  cache->put("key2", "value2");

  cache->clear();
  std::string result;
  EXPECT_FALSE(cache->get("key1", result));
  EXPECT_FALSE(cache->get("key2", result));
  EXPECT_EQ(cache->size(), 0);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
