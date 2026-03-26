// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/common/js_call_native_frequency_monitor.h"

#include <utility>
#include <vector>

#include "base/include/timer/time_utils.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace runtime {
namespace {

constexpr size_t kStacksMaxLen = 4096;
constexpr size_t kStacksSummaryMaxBytes = 16 * 1024;

inline std::string Truncate(std::string_view s, size_t max_len) {
  if (s.size() <= max_len) {
    return std::string(s.data(), s.size());
  }
  return std::string(s.data(), max_len);
}

}  // namespace

JsCallNativeFrequencyMonitor::JsCallNativeFrequencyMonitor(
    uint32_t threshold_per_method)
    : window_ms_(tasm::LynxEnv::GetInstance()
                     .GetJSCallNativeFrequencyMonitorWindowMs()),
      threshold_per_method_(threshold_per_method),
      cooldown_ms_(tasm::LynxEnv::GetInstance()
                       .GetJSCallNativeFrequencyMonitorCooldownMs()),
      window_start_ms_(0),
      last_report_ms_(0) {
  auto& env = tasm::LynxEnv::GetInstance();
  if (threshold_per_method_ <= 0) {
    threshold_per_method_ =
        env.GetJSCallNativeFrequencyMonitorThresholdCommon();
  }
}

std::optional<base::LynxError> JsCallNativeFrequencyMonitor::Record(
    std::string_view call_api_name, std::string_view method_name,
    std::string_view stacks) {
  const uint64_t now_ms = base::CurrentSystemTimeMilliseconds();
  if (window_start_ms_ == 0 || now_ms - window_start_ms_ >= window_ms_) {
    ResetWindow(now_ms);
  }

  std::string method_key(method_name.data(), method_name.size());
  auto& stats = per_method_stats_[method_key];
  stats.count_in_window += 1;

  std::string stack_key =
      stacks.empty() ? std::string("") : Truncate(stacks, kStacksMaxLen);
  stats.stacks_counts[stack_key] += 1;

  if (stats.count_in_window < threshold_per_method_) {
    return std::nullopt;
  }
  if (last_report_ms_ != 0 && now_ms - last_report_ms_ < cooldown_ms_) {
    return std::nullopt;
  }
  last_report_ms_ = now_ms;

  std::string api_name(call_api_name.data(), call_api_name.size());
  std::string msg;
  msg.reserve(api_name.size() + method_key.size() + 48);
  msg.append(api_name);
  msg.append(" called too frequently. method_name:");
  msg.append(method_key);

  base::LynxError error(error::E_BTS_JS_CALL_NATIVE_FUNCTION_TOO_FREQUENCY,
                        std::move(msg),
                        std::string("Please throttle related calls."),
                        base::LynxErrorLevel::Warn);
  error.AddContextInfo("call_api_name", api_name);
  error.AddContextInfo("method_name", method_key);
  error.AddContextInfo("count_in_window",
                       std::to_string(stats.count_in_window));

  const std::string stacks_summary =
      BuildStacksSummary(stats.stacks_counts, kStacksSummaryMaxBytes);
  if (!stacks_summary.empty()) {
    error.AddContextInfo("stacks", stacks_summary);
  }

  stats.count_in_window = 0;
  stats.stacks_counts.clear();
  return error;
}

void JsCallNativeFrequencyMonitor::ResetWindow(uint64_t now_ms) {
  window_start_ms_ = now_ms;
  per_method_stats_.clear();
}

std::string JsCallNativeFrequencyMonitor::BuildStacksSummary(
    const std::unordered_map<std::string, uint32_t>& stacks_counts,
    size_t max_bytes) {
  if (stacks_counts.empty() || max_bytes == 0) {
    return "";
  }
  std::vector<std::pair<std::string, uint32_t>> items;
  items.reserve(stacks_counts.size());
  for (const auto& it : stacks_counts) {
    std::pair<std::string, uint32_t> key(it.first, it.second);
    items.emplace_back();
    size_t i = items.size() - 1;
    while (i > 0 && items[i - 1].second < key.second) {
      items[i] = std::move(items[i - 1]);
      --i;
    }
    items[i] = std::move(key);
  }

  std::string out;
  for (const auto& it : items) {
    const std::string count = std::to_string(it.second);
    const size_t needed = count.size() + 2 + it.first.size() + 2;
    if (out.size() + needed > max_bytes) {
      break;
    }
    out.append(count);
    out.append("x\n");
    out.append(it.first);
    out.append("\n\n");
  }
  return out;
}

}  // namespace runtime
}  // namespace lynx
