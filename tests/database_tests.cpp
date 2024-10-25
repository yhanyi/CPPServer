#include "../src/cache.hpp"
#include "../src/database.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <vector>

// Helper function for database configuration
inline std::string getTestDbConfig(const char *key, const char *default_value) {
  const char *value = std::getenv(key);
  return value ? std::string(value) : std::string(default_value);
}

// Test fixture for database-only tests
class DatabaseTest : public ::testing::Test {
protected:
  std::unique_ptr<DatabaseConnection> db;

  void SetUp() override {
    // Create fresh database connection for each test
    db = std::make_unique<DatabaseConnection>(
        getTestDbConfig("POSTGRES_HOST", "localhost"),
        getTestDbConfig("POSTGRES_PORT", "5432"),
        getTestDbConfig("POSTGRES_DB", "cache_db"));

    // Clear any existing data
    pqxx::work txn(*db->get_connection());
    txn.exec("DELETE FROM cache_entries");
    txn.commit();
  }

  void TearDown() override { db.reset(); }

  bool entry_exists(const std::string &key) {
    try {
      pqxx::work txn(*db->get_connection());
      auto result = txn.exec_params(
          "SELECT COUNT(*) FROM cache_entries WHERE key = $1", key);
      txn.commit();
      return result[0][0].as<int>() > 0;
    } catch (const std::exception &e) {
      std::cerr << "Error checking entry existence: " << e.what() << std::endl;
      return false;
    }
  }
};

// Test fixture for cache persistence tests
class CachePersistenceTest : public ::testing::Test {
protected:
  std::unique_ptr<LRUCache<std::string, std::string>> cache;

  void SetUp() override {
    // Clear database first
    {
      DatabaseConnection db;
      pqxx::work txn(*db.get_connection());
      txn.exec("DELETE FROM cache_entries");
      txn.commit();
    }

    // Create cache instance
    cache = std::make_unique<LRUCache<std::string, std::string>>(
        3, std::chrono::seconds(5));
  }

  void TearDown() override { cache.reset(); }
};

// Database-specific tests
TEST_F(DatabaseTest, BasicPutGet) {
  auto future = std::chrono::system_clock::now() + std::chrono::hours(1);
  EXPECT_TRUE(db->put("test_key", "test_value", future));

  auto result = db->get("test_key");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "test_value");
}

TEST_F(DatabaseTest, ExpiredEntry) {
  auto past = std::chrono::system_clock::now() - std::chrono::hours(1);
  EXPECT_TRUE(db->put("expired_key", "expired_value", past));
  EXPECT_FALSE(db->get("expired_key").has_value());
}

TEST_F(DatabaseTest, CleanupExpired) {
  auto future = std::chrono::system_clock::now() + std::chrono::hours(1);
  auto past = std::chrono::system_clock::now() - std::chrono::hours(1);

  db->put("future_key", "future_value", future);
  db->put("past_key", "past_value", past);

  db->cleanup_expired();

  EXPECT_TRUE(entry_exists("future_key"));
  EXPECT_FALSE(entry_exists("past_key"));
}

TEST_F(DatabaseTest, ConcurrentAccess) {
  const int NUM_THREADS = 10;
  std::vector<std::thread> threads;
  auto future = std::chrono::system_clock::now() + std::chrono::hours(1);

  // Concurrent writes and reads
  for (int i = 0; i < NUM_THREADS; i++) {
    threads.emplace_back([this, i, future]() {
      std::string key = "key" + std::to_string(i);
      std::string value = "value" + std::to_string(i);
      EXPECT_TRUE(db->put(key, value, future));
      auto result = db->get(key);
      EXPECT_TRUE(result.has_value());
      if (result.has_value()) {
        EXPECT_EQ(*result, value);
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }
}

// Cache persistence tests
TEST_F(CachePersistenceTest, DatabasePersistence) {
  cache->put("persistent_key", "persistent_value");

  // Create new cache instance
  cache.reset();
  cache = std::make_unique<LRUCache<std::string, std::string>>(
      3, std::chrono::seconds(5));

  std::string result;
  EXPECT_TRUE(cache->get("persistent_key", result));
  EXPECT_EQ(result, "persistent_value");
}

TEST_F(CachePersistenceTest, ExpiryHandling) {
  cache->put("expiring_key", "value", std::chrono::seconds(1));

  std::string result;
  EXPECT_TRUE(cache->get("expiring_key", result));
  EXPECT_EQ(result, "value");

  std::this_thread::sleep_for(std::chrono::seconds(2));
  EXPECT_FALSE(cache->get("expiring_key", result));
}

TEST_F(CachePersistenceTest, CacheAndDatabaseSync) {
  // Test cache to database sync
  cache->put("sync_test", "sync_value");
  {
    DatabaseConnection direct_db;
    auto result = direct_db.get("sync_test");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "sync_value");
  }

  // Test database to cache sync
  {
    DatabaseConnection direct_db;
    auto future = std::chrono::system_clock::now() + std::chrono::hours(1);
    direct_db.put("sync_test", "modified_value", future);
  }

  std::string cache_result;
  EXPECT_TRUE(cache->get("sync_test", cache_result));
  EXPECT_EQ(cache_result, "modified_value");
}

TEST_F(CachePersistenceTest, LargeDataSet) {
  const int NUM_ENTRIES = 100;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 1000);

  std::vector<std::string> keys;
  for (int i = 0; i < NUM_ENTRIES; i++) {
    std::string key = "key" + std::to_string(dis(gen));
    std::string value = "value" + std::to_string(dis(gen));
    cache->put(key, value);
    keys.push_back(key);
  }

  // Verify some random keys exist
  std::string result;
  int found = 0;
  for (const auto &key : keys) {
    if (cache->get(key, result)) {
      found++;
    }
  }

  // Should find at least some entries (considering capacity limits)
  EXPECT_GT(found, 0);
  EXPECT_LE(found, 1024); // Default cache size
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
