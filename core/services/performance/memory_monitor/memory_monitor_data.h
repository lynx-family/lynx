// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_DATA_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_MEMORY_MONITOR_DATA_H_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <string>

#include "base/include/fml/time/time_point.h"

namespace lynx {
namespace tasm {
namespace performance {

inline int64_t MemoryNowMs() {
  return fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
}

// These bits describe non-overlapping *page* sources. Shared BTS heap is never
// a page source: only its attributed slot is. The global event replaces all
// these slots with the unique shared heap once.
enum MemorySource : uint32_t {
  kElementMemory = 1,
  kMtsMemory = 2,
  kBtsMemory = 4,
  kUiMemory = 8,
  kCoreMemory = kElementMemory | kMtsMemory | kBtsMemory,
  kFullMemory = kCoreMemory | kUiMemory,
};

struct PageMemoryUsage {
  int64_t total_bytes{0};
  // The BTS bytes ALREADY included in total_bytes. Do not subtract a newer
  // slot from an older total: total = total - old_bts + new_bts.
  int64_t bts_bytes{0};
  int64_t element_bytes{0};
  int64_t mts_bytes{0};
  int64_t ui_bytes{0};
  uint32_t available{0};
  // An applicable but unsupported/unsampled source is not eligible. In
  // particular, shared V8/JSVM pages cannot claim complete page attribution.
  uint32_t eligible{0};
  uint32_t required{kFullMemory};
  bool bts_shared{false};
  bool arithmetic_valid{true};

  bool Valid(uint32_t scope) const {
    const auto needed = required & scope;
    return arithmetic_valid && (eligible & needed) == needed &&
           (available & needed) == needed;
  }
  int64_t CoreBytes() const { return total_bytes - ui_bytes; }
};

inline bool AddMemoryBytes(int64_t value, int64_t& sum) {
  if (value < 0 || sum < 0 ||
      sum > std::numeric_limits<int64_t>::max() - value) {
    return false;
  }
  sum += value;
  return true;
}

// Keep query/fragment data out of telemetry (tokens, user IDs, etc.). Dynamic
// path-to-route normalization is a host responsibility; this helper must not
// invent route identities by guessing which path segments are identifiers.
inline std::string MemoryUrl(const std::string& url) {
  return url.substr(0, url.find_first_of("?#"));
}

inline constexpr std::array<int64_t, 12> kPageMemoryOffsetsMs = {
    0,     5000,  10000,  15000,  20000,  25000,
    30000, 60000, 120000, 300000, 600000, 1800000};
inline constexpr int64_t kEarlyMemoryWindowMs = 30000;

// Equal-weight statistics over the early scheduled points only. Neither
// producer callbacks nor teardown values may enter this accumulator. A short
// page with just its anchor sample can contribute, but count=1 does not imply
// coverage of the rest of its lifetime.
struct EarlyMemorySamples {
  int64_t last_ms{-1};
  int64_t peak_bytes{0};
  int64_t max_gap_ms{0};
  double sum_bytes{0};
  // Keep failed attempts as well as valid points: retries must not replace a
  // failed observation and make the same instance appear fully collected.
  uint32_t attempted_mask{0};
  uint32_t scheduled_mask{0};
  int32_t count{0};

  void Observe(int64_t at_ms, int64_t bytes, bool available, int index) {
    if (index < 0 || index >= 7) return;
    const uint32_t bit = 1u << index;
    if (attempted_mask & bit) return;
    attempted_mask |= bit;
    if (!available || bytes < 0 || at_ms < 0 || at_ms <= last_ms ||
        std::abs(at_ms - kPageMemoryOffsetsMs[index]) > 1000)
      return;
    scheduled_mask |= bit;
    sum_bytes += bytes;
    ++count;
    if (last_ms >= 0) {
      max_gap_ms = std::max(max_gap_ms, at_ms - last_ms);
    }
    peak_bytes = std::max(peak_bytes, bytes);
    last_ms = at_ms;
  }

  bool Complete(uint32_t required_mask) const {
    return count > 0 && required_mask != 0 && attempted_mask == required_mask &&
           scheduled_mask == required_mask && max_gap_ms <= 6000;
  }
  // Each instance publishes sum/count once. Group by URL and take P95 of those
  // means, not P95 of all raw events or SUM(sum)/SUM(count): those overweight
  // longer-lived pages. This is not a time integral or an exit measurement.
  double Mean() const { return count > 0 ? sum_bytes / count : 0; }
};

// Weighted reservoir, O(1) auxiliary space. Once all candidates are scanned,
// P(select i)=weight_i/total. For an attributed quantity x_i, emit x_i/P(i).
// Uniform page sampling uses weight=1; residual pages and VMs use byte weights.
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
