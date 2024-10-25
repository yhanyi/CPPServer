#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdlib>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
#include <pqxx/pqxx>
#include <pwd.h>
#include <string>
#include <unistd.h>

class DatabaseConnection {
private:
  std::unique_ptr<pqxx::connection> conn;
  std::mutex db_mutex;
  std::string db_host, db_port, db_name, db_user, db_password;

  std::string get_system_username() {
    // Try getenv first (most reliable on macOS)
    if (const char *user_env = std::getenv("USER")) {
      return std::string(user_env);
    }

    // Try pwd entry
    if (const struct passwd *pw = getpwuid(getuid())) {
      return std::string(pw->pw_name);
    }

    throw std::runtime_error("Could not determine system username");
  }

public:
  DatabaseConnection(const std::string &host = "localhost",
                     const std::string &port = "5432",
                     const std::string &dbname = "cache_db",
                     const std::string &user = "",
                     const std::string &password = "") {
    try {
      // Get system username if none provided
      std::string actual_user = user.empty() ? get_system_username() : user;

      // Build connection string without password for local connection
      std::string conn_string = "host=" + host + " port=" + port +
                                " dbname=" + dbname + " user=" + actual_user;

      // Add password only if provided
      if (!password.empty()) {
        conn_string += " password=" + password;
      }

      std::cout << "Attempting to connect with user: " << actual_user
                << std::endl;
      conn = std::make_unique<pqxx::connection>(conn_string);

      // Create cache table if it doesn't exist
      pqxx::work txn(*conn);
      txn.exec("CREATE TABLE IF NOT EXISTS cache_entries ("
               "key TEXT PRIMARY KEY,"
               "value TEXT NOT NULL,"
               "expiry TIMESTAMP NOT NULL,"
               "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
               ")");
      txn.commit();
    } catch (const std::exception &e) {
      throw std::runtime_error("Database connection failed: " +
                               std::string(e.what()));
    }
  }

  pqxx::connection *get_connection() { return conn.get(); }

  bool put(const std::string &key, const std::string &value,
           const std::chrono::system_clock::time_point &expiry) {
    std::lock_guard<std::mutex> lock(db_mutex);
    try {
      pqxx::work txn(*conn);

      // Convert time_point to PostgreSQL timestamp string
      std::time_t t = std::chrono::system_clock::to_time_t(expiry);
      std::stringstream ss;
      ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");

      txn.exec_params("INSERT INTO cache_entries (key, value, expiry) "
                      "VALUES ($1, $2, $3::timestamp) "
                      "ON CONFLICT (key) DO UPDATE "
                      "SET value = EXCLUDED.value, "
                      "expiry = EXCLUDED.expiry",
                      key, value, ss.str());

      txn.commit();
      return true;
    } catch (const std::exception &e) {
      std::cerr << "Database error: " << e.what() << std::endl;
      return false;
    }
  }

  std::optional<std::string> get(const std::string &key) {
    std::lock_guard<std::mutex> lock(db_mutex);
    try {
      pqxx::work txn(*conn);

      auto result = txn.exec_params(
          "SELECT value FROM cache_entries "
          "WHERE key = $1 AND expiry > CURRENT_TIMESTAMP::timestamp",
          key);

      txn.commit();

      if (result.empty()) {
        return std::nullopt;
      }

      return result[0][0].as<std::string>();
    } catch (const std::exception &e) {
      std::cerr << "Database error: " << e.what() << std::endl;
      return std::nullopt;
    }
  }

  void cleanup_expired() {
    std::lock_guard<std::mutex> lock(db_mutex);
    try {
      pqxx::work txn(*conn);
      txn.exec("DELETE FROM cache_entries WHERE expiry <= "
               "CURRENT_TIMESTAMP::timestamp");
      txn.commit();
    } catch (const std::exception &e) {
      std::cerr << "Database cleanup error: " << e.what() << std::endl;
    }
  }
};

#endif
