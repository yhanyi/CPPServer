#ifndef DATABASE_HELPER_HPP
#define DATABASE_HELPER_HPP

#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <pqxx/pqxx>
#include <random>

class TestDatabase {
private:
  static std::unique_ptr<pqxx::connection> conn;
  static std::mutex conn_mutex;

  static std::string get_test_db_name() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    return "test_db_" + std::to_string(dis(gen));
  }

public:
  static void SetUpTestCase() {
    try {
      // Create test database
      std::string test_db = get_test_db_name();
      pqxx::connection admin_conn("dbname=postgres");
      pqxx::work txn(admin_conn);
      txn.exec("CREATE DATABASE " + test_db);
      txn.commit();

      // Connect to test database
      conn = std::make_unique<pqxx::connection>("dbname=" + test_db);

      // Create schema
      pqxx::work schema_txn(*conn);
      schema_txn.exec("CREATE TABLE IF NOT EXISTS cache_entries ("
                      "key TEXT PRIMARY KEY,"
                      "value TEXT NOT NULL,"
                      "expiry TIMESTAMP NOT NULL,"
                      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");
      schema_txn.commit();
    } catch (const std::exception &e) {
      std::cerr << "Test database setup failed: " << e.what() << std::endl;
      throw;
    }
  }

  static void TearDownTestCase() {
    std::string db_name = conn->dbname();
    conn.reset();

    // Drop test database
    try {
      pqxx::connection admin_conn("dbname=postgres");
      pqxx::work txn(admin_conn);
      txn.exec("SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE "
               "datname = '" +
               db_name + "'");
      txn.exec("DROP DATABASE IF EXISTS " + db_name);
      txn.commit();
    } catch (const std::exception &e) {
      std::cerr << "Test database cleanup failed: " << e.what() << std::endl;
    }
  }

  static pqxx::connection *get_connection() { return conn.get(); }

  static void clean_table() {
    std::lock_guard<std::mutex> lock(conn_mutex);
    pqxx::work txn(*conn);
    txn.exec("DELETE FROM cache_entries");
    txn.commit();
  }
};

// Initialize static members
std::unique_ptr<pqxx::connection> TestDatabase::conn;
std::mutex TestDatabase::conn_mutex;

class DatabaseTestFixture : public ::testing::Test {
protected:
  void SetUp() override { TestDatabase::clean_table(); }
};

#endif
