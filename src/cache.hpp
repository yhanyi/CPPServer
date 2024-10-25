#ifndef CACHE_HPP
#define CACHE_HPP

#include "database.hpp"
#include "metrics.hpp"
#include <algorithm>
#include <chrono>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>

template <typename K, typename V> class LRUCache {
private:
  struct CacheEntry {
    V value;
    std::chrono::steady_clock::time_point expiry;
    CacheEntry(V v, std::chrono::seconds ttl)
        : value(v), expiry(std::chrono::steady_clock::now() + ttl) {}
  };

  size_t capacity;
  std::mutex cache_mutex;
  std::unordered_map<K, CacheEntry> cache_map;
  std::list<K> lru_list;
  std::chrono::seconds default_ttl;
  std::unique_ptr<CacheMetrics> metrics;
  std::unique_ptr<DatabaseConnection> db;
  std::atomic<bool> cleanup_running;
  std::unique_ptr<std::thread> cleanup_thread;

  void evict() {
    if (!lru_list.empty()) {
      auto last = lru_list.back();
      lru_list.pop_back();
      cache_map.erase(last);
      metrics->record_eviction();
      metrics->update_size(cache_map.size());
    }
  }

public:
  LRUCache(size_t size = 1024,
           std::chrono::seconds ttl = std::chrono::seconds(300))
      : capacity(size), default_ttl(ttl),
        metrics(std::make_unique<CacheMetrics>()),
        db(std::make_unique<DatabaseConnection>()), cleanup_running(false) {
    metrics->update_size(0);
    metrics->update_memory(0);
    start_cleanup_thread();
  }

  ~LRUCache() {
    cleanup_running = false;
    if (cleanup_thread && cleanup_thread->joinable()) {
      cleanup_thread->join();
    }
  }

  DatabaseConnection *get_db() { return db.get(); }

  void start_cleanup_thread() {
    cleanup_running = true;
    cleanup_thread = std::make_unique<std::thread>([this]() {
      while (cleanup_running) {
        db->cleanup_expired();
        std::this_thread::sleep_for(std::chrono::minutes(5));
      }
    });
  }

  void put(const K &key, const V &value,
           std::chrono::seconds ttl = std::chrono::seconds(0)) {
    if (ttl.count() == 0)
      ttl = default_ttl;

    auto expiry = std::chrono::system_clock::now() + ttl;

    auto future = std::async(std::launch::async, [this, key, value, expiry]() {
      db->put(key, value, expiry);
    });

    std::lock_guard<std::mutex> lock(cache_mutex);

    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
      it->second = CacheEntry(value, ttl);
      auto list_it = std::find(lru_list.begin(), lru_list.end(), key);
      if (list_it != lru_list.end()) {
        lru_list.erase(list_it);
      }
      lru_list.push_front(key);
      return;
    }

    if (cache_map.size() >= capacity) {
      evict();
    }

    cache_map.insert({key, CacheEntry(value, ttl)});
    lru_list.push_front(key);
  }

  bool get(const K &key, V &value) {
    {
      std::lock_guard<std::mutex> lock(cache_mutex);

      auto it = cache_map.find(key);
      if (it != cache_map.end()) {
        if (std::chrono::steady_clock::now() <= it->second.expiry) {
          metrics->record_hit();
          value = it->second.value;
          auto list_it = std::find(lru_list.begin(), lru_list.end(), key);
          if (list_it != lru_list.end()) {
            lru_list.erase(list_it);
          }
          lru_list.push_front(key);
          return true;
        }
        cache_map.erase(it);
        metrics->record_expired();
        metrics->update_size(cache_map.size());
      }
    }
    auto db_value = db->get(key);
    if (db_value) {
      value = *db_value;
      metrics->record_hit();
      put(key, value);
      return true;
    }
    metrics->record_miss();
    return false;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    cache_map.clear();
    lru_list.clear();
  }

  size_t size() const { return cache_map.size(); }
};

#endif
