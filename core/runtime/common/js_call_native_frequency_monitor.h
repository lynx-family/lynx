// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_COMMON_JS_CALL_NATIVE_FREQUENCY_MONITOR_H_
#define CORE_RUNTIME_COMMON_JS_CALL_NATIVE_FREQUENCY_MONITOR_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "base/include/debug/lynx_error.h"

namespace lynx {
namespace runtime {

class JsCallNativeFrequencyMonitor {
 public:
  explicit JsCallNativeFrequencyMonitor(uint32_t threshold_per_method);

  std::optional<base::LynxError> Record(std::string_view call_api_name,
                                        std::string_view method_name,
                                        std::string_view stacks = "");

 private:
  struct MethodStats {
    uint32_t count_in_window = 0;
    std::unordered_map<std::string, uint32_t> stacks_counts;
  };

  void ResetWindow(uint64_t now_ms);
  static std::string BuildStacksSummary(
      const std::unordered_map<std::string, uint32_t>& stacks_counts,
      size_t max_bytes);

  uint32_t window_ms_;
  uint32_t threshold_per_method_;
  uint32_t cooldown_ms_;

  uint64_t window_start_ms_;
  uint64_t last_report_ms_;
  std::unordered_map<std::string, MethodStats> per_method_stats_;
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_COMMON_JS_CALL_NATIVE_FREQUENCY_MONITOR_H_
