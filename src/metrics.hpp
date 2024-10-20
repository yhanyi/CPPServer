#ifndef METRICS_HPP
#define METRICS_HPP

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/gauge.h>
#include <prometheus/registry.h>

class CacheMetrics {
private:
  std::shared_ptr<prometheus::Registry> registry;
  prometheus::Exposer exposer;

  // Families
  prometheus::Family<prometheus::Counter> &cache_hits_family;
  prometheus::Family<prometheus::Counter> &cache_misses_family;
  prometheus::Family<prometheus::Counter> &evictions_family;
  prometheus::Family<prometheus::Counter> &expired_items_family;
  prometheus::Family<prometheus::Gauge> &cache_size_family;
  prometheus::Family<prometheus::Gauge> &memory_usage_family;

  // Actual metrics
  prometheus::Counter &cache_hits_counter;
  prometheus::Counter &cache_misses_counter;
  prometheus::Counter &evictions_counter;
  prometheus::Counter &expired_items_counter;
  prometheus::Gauge &cache_size_gauge;
  prometheus::Gauge &memory_usage_gauge;

public:
  CacheMetrics(const std::string &metrics_address = "0.0.0.0:9091")
      : registry(std::make_shared<prometheus::Registry>()),
        exposer(metrics_address),
        cache_hits_family(prometheus::BuildCounter()
                              .Name("cache_hits_total")
                              .Help("Total number of cache hits")
                              .Register(*registry)),
        cache_misses_family(prometheus::BuildCounter()
                                .Name("cache_misses_total")
                                .Help("Total number of cache misses")
                                .Register(*registry)),
        evictions_family(prometheus::BuildCounter()
                             .Name("cache_evictions_total")
                             .Help("Total number of cache evictions")
                             .Register(*registry)),
        expired_items_family(prometheus::BuildCounter()
                                 .Name("cache_expired_total")
                                 .Help("Total number of expired items")
                                 .Register(*registry)),
        cache_size_family(prometheus::BuildGauge()
                              .Name("cache_size_bytes")
                              .Help("Current size of cache in bytes")
                              .Register(*registry)),
        memory_usage_family(prometheus::BuildGauge()
                                .Name("cache_memory_usage_bytes")
                                .Help("Current memory usage in bytes")
                                .Register(*registry)),
        cache_hits_counter(cache_hits_family.Add({})),
        cache_misses_counter(cache_misses_family.Add({})),
        evictions_counter(evictions_family.Add({})),
        expired_items_counter(expired_items_family.Add({})),
        cache_size_gauge(cache_size_family.Add({})),
        memory_usage_gauge(memory_usage_family.Add({})) {
    exposer.RegisterCollectable(registry);
  }

  void record_hit() { cache_hits_counter.Increment(); }
  void record_miss() { cache_misses_counter.Increment(); }
  void record_eviction() { evictions_counter.Increment(); }
  void record_expired() { expired_items_counter.Increment(); }
  void update_size(double size) { cache_size_gauge.Set(size); }
  void update_memory(double memory) { memory_usage_gauge.Set(memory); }
};

#endif
