#include "../src/database.hpp"
#include "database_helper.hpp"
#include <future>

class DatabaseTest : public DatabaseTestFixture {
protected:
  std::unique_ptr<DatabaseConnection> db;

  void SetUp() override {
    DatabaseTestFixture::SetUp();
    db = std::make_unique<DatabaseConnection>();
  }
};

TEST_F(DatabaseTest, BasicPutGet) {
  auto future = std::chrono::system_clock::now() + std::chrono::seconds(5);
  ASSERT_TRUE(db->put("test_key", "test_value", future));

  auto result = db->get("test_key");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "test_value");
}

TEST_F(DatabaseTest, ConcurrentAccess) {
  constexpr int NUM_THREADS = 10;
  std::vector<std::future<void>> futures;

  for (int i = 0; i < NUM_THREADS; i++) {
    futures.push_back(std::async(std::launch::async, [this, i]() {
      auto expiry = std::chrono::system_clock::now() + std::chrono::seconds(5);
      std::string key = "key" + std::to_string(i);
      std::string value = "value" + std::to_string(i);

      EXPECT_TRUE(db->put(key, value, expiry));

      auto result = db->get(key);
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(*result, value);
    }));
  }

  for (auto &f : futures) {
    f.wait();
  }
}

// Add more tests here...

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  TestDatabase::SetUpTestCase();
  int result = RUN_ALL_TESTS();
  TestDatabase::TearDownTestCase();
  return result;
}
