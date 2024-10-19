#ifndef CACHE_HPP
#define CACHE_HPP

#include <chrono>
#include <list>
#include <mutex>
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

  void evict() {
    if (!lru_list.empty()) {
      auto last = lru_list.back();
      lru_list.pop_back();
      cache_map.erase(last);
    }
  }

public:
  LRUCache(size_t size = 1024,
           std::chrono::seconds ttl = std::chrono::seconds(300))
      : capacity(size), default_ttl(ttl) {}

  void put(const K &key, const V &value,
           std::chrono::seconds ttl = std::chrono::seconds(0)) {
    std::lock_guard<std::mutex> lock(cache_mutex);

    if (ttl.count() == 0)
      ttl = default_ttl;

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
    std::lock_guard<std::mutex> lock(cache_mutex);

    auto it = cache_map.find(key);
    if (it == cache_map.end()) {
      return false;
    }

    if (std::chrono::steady_clock::now() > it->second.expiry) {
      cache_map.erase(it);
      auto list_it = std::find(lru_list.begin(), lru_list.end(), key);
      if (list_it != lru_list.end()) {
        lru_list.erase(list_it);
      }
      return false;
    }

    value = it->second.value;

    auto list_it = std::find(lru_list.begin(), lru_list.end(), key);
    if (list_it != lru_list.end()) {
      lru_list.erase(list_it);
    }
    lru_list.push_front(key);

    return true;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    cache_map.clear();
    lru_list.clear();
  }

  size_t size() const { return cache_map.size(); }
};

#endif
