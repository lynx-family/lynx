// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_DATA_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_DATA_H_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include "base/include/fml/time/time_point.h"

namespace lynx {
namespace tasm {
namespace performance {

inline int64_t MemoryNowMs() {
  return fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
}

struct PageMemoryUsage {
  int64_t total_bytes{0};
  // The BTS bytes ALREADY included in total_bytes. Do not subtract a newer
  // slot from an older total: total = total - old_bts + new_bts.
  int64_t bts_bytes{0};
  int64_t element_bytes{0};
  int64_t mts_bytes{0};
  int64_t ui_bytes{0};
  int32_t element_count{0};
  bool bts_shared{false};
};

struct InstanceMemoryUsage {
  int32_t instance_id{-1};
  std::string page_id;
  std::string url;
  PageMemoryUsage page;
  // Whole heap of the BTS VM used by this page. For shared VMs this differs
  // from page.bts_bytes, which contains only the page-attributed slot.
  int64_t bts_heap_bytes{0};
  std::string bts_runtime_group_id;
};

enum class MemoryUsageQueryStatus : int32_t {
  kOk = 0,
  kMonitoringDisabled = 1,
  kInvalidInstance = 2,
};

struct InstanceMemoryUsageResult {
  MemoryUsageQueryStatus status{MemoryUsageQueryStatus::kOk};
  PageMemoryUsage page;
  int64_t bts_heap_bytes{0};
};

struct GlobalMemoryUsage {
  // Aggregated page-owned categories plus each active BTS VM heap exactly once.
  // Bundle-local MTS pools are process overhead, not page-owned MTS memory, so
  // they are intentionally excluded from this query result.
  PageMemoryUsage total;
  std::vector<InstanceMemoryUsage> instances;
};

struct MTSRuntimePoolState {
  int32_t pool_instance_id{-1};
  int32_t context_type{0};
  int64_t created_at_ms{0};
  uint64_t runtime_count{0};
  int64_t heap_bytes{0};
  bool page_key_resolved{false};
  std::string template_url;
  std::string page_key;
};

// Offsets from first loadBundle completion, independent of visibility.
// Each point is attempted at most once; missed points are not backfilled.
// Residency uses page creation + 10min instead of this sampling anchor.
inline constexpr std::array<int64_t, 15> kPageMemoryOffsetsMs = {
    0,     1000,  2000,  3000,   5000,   8000,   12000,  16000,
    20000, 40000, 60000, 120000, 300000, 600000, 1800000};
inline constexpr size_t kEarlyMemorySampleCount = 9;
inline constexpr int64_t kEarlyMemoryWindowMs =
    kPageMemoryOffsetsMs[kEarlyMemorySampleCount - 1];
static_assert(kEarlyMemorySampleCount <= std::numeric_limits<uint32_t>::digits);

// Equal-weight statistics over the early scheduled points only. Neither
// producer callbacks nor teardown values may enter this accumulator. A short
// page with just its anchor sample can contribute, but count=1 does not imply
// coverage of the rest of its lifetime.
struct EarlyMemorySamples {
  int64_t peak_bytes{0};
  double sum_bytes{0};
  // Keep failed attempts as well as valid points: retries must not replace a
  // missed observation and make the same instance appear fully collected.
  uint32_t attempted_mask{0};
  uint32_t scheduled_mask{0};
  int32_t count{0};

  void Observe(int64_t bytes, int index) {
    if (index < 0 || static_cast<size_t>(index) >= kEarlyMemorySampleCount)
      return;
    const uint32_t bit = 1u << index;
    if (attempted_mask & bit) return;
    attempted_mask |= bit;
    if (bytes < 0) return;
    scheduled_mask |= bit;
    sum_bytes += bytes;
    ++count;
    peak_bytes = std::max(peak_bytes, bytes);
  }

  bool Complete(uint32_t required_mask) const {
    return count > 0 && required_mask != 0 && attempted_mask == required_mask &&
           scheduled_mask == required_mask;
  }
  // Each instance publishes one mean. Group by page key and take P95 of those
  // means, not P95 of all raw events: that overweights longer-lived pages.
  // This is not a time integral or an exit measurement.
  double Mean() const { return count > 0 ? sum_bytes / count : 0; }
};

// Weighted reservoir, O(1) auxiliary space. Once all candidates are scanned,
// P(select i)=weight_i/total. For an attributed quantity x_i, emit x_i/P(i).
// Active pages and registered shared VMs use weight=1, including missing
// memory values. Exited pages use positive, post-exit full-GC residual bytes.
// These draw weights differ from the emitted inverse selection weights:
// N for pages, V for VMs, total residual / selected residual for exited pages.
class MemoryReservoir {
 public:
  template <typename Random>
  bool Consider(double weight, Random& random) {
    if (!(weight > 0) || !std::isfinite(weight)) return false;
    total_ += weight;
    return std::uniform_real_distribution<double>(0, total_)(random) < weight;
  }
  double total() const { return total_; }

 private:
  double total_{0};
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_DATA_H_
