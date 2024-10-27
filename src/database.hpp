#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdlib>
#include <future>
#include <iomanip>
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

    return "postgres"; // Default fallback for Docker environment
  }

  std::string get_env_or_default(const char *env_var,
                                 const std::string &default_value) {
    const char *value = std::getenv(env_var);
    return value ? std::string(value) : default_value;
  }

public:
  DatabaseConnection(const std::string &host = "", const std::string &port = "",
                     const std::string &dbname = "",
                     const std::string &user = "",
                     const std::string &password = "") {
    try {
      // Use environment variables with fallbacks
      std::string actual_host =
          !host.empty() ? host
                        : get_env_or_default("POSTGRES_HOST", "localhost");
      std::string actual_port =
          !port.empty() ? port : get_env_or_default("POSTGRES_PORT", "5432");
      std::string actual_dbname =
          !dbname.empty() ? dbname
                          : get_env_or_default("POSTGRES_DB", "cache_db");
      std::string actual_user =
          !user.empty()
              ? user
              : get_env_or_default("POSTGRES_USER", get_system_username());
      std::string actual_password =
          !password.empty() ? password
                            : get_env_or_default("POSTGRES_PASSWORD", "");

      // Build connection string
      std::string conn_string = "host=" + actual_host + " port=" + actual_port +
                                " dbname=" + actual_dbname +
                                " user=" + actual_user;

      // Add password if available
      if (!actual_password.empty()) {
        conn_string += " password=" + actual_password;
      }

      std::cout << "Attempting database connection..." << std::endl;
      std::cout << "Host: " << actual_host << ", Port: " << actual_port
                << ", DB: " << actual_dbname << ", User: " << actual_user
                << std::endl;

      // Try to connect with exponential backoff
      int retry_count = 0;
      const int max_retries = 5;
      std::chrono::seconds wait_time(1);

      while (retry_count < max_retries) {
        try {
          conn = std::make_unique<pqxx::connection>(conn_string);
          break;
        } catch (const std::exception &e) {
          retry_count++;
          if (retry_count == max_retries) {
            throw;
          }
          std::cerr << "Connection attempt " << retry_count
                    << " failed: " << e.what() << ". Retrying in "
                    << wait_time.count() << " seconds..." << std::endl;
          std::this_thread::sleep_for(wait_time);
          wait_time *= 2; // Exponential backoff
        }
      }

      // Create cache table if it doesn't exist
      pqxx::work txn(*conn);
      txn.exec("CREATE TABLE IF NOT EXISTS cache_entries ("
               "key TEXT PRIMARY KEY,"
               "value TEXT NOT NULL,"
               "expiry TIMESTAMP NOT NULL,"
               "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
               ")");
      txn.commit();

      std::cout << "Database connection and initialization successful!"
                << std::endl;

    } catch (const std::exception &e) {
      throw std::runtime_error("Database connection failed: " +
                               std::string(e.what()));
    }
  }

  pqxx::connection *get_connection() { return conn.get(); }

  // Rest of your existing methods remain the same...
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
