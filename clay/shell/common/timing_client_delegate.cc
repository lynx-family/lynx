// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/timing_client_delegate.h"

namespace clay {

namespace {
constexpr const char* kSetupFlag = "clay_setup";
constexpr const char* kUpdateFlag = "clay_update";
constexpr const char* kForceUpdateFlag = "clay_force_update";
}  // namespace

void TimingClientDelegate::ReportTiming(
    const std::unordered_map<std::string, int64_t>& timing,
    const std::string& flag) {
  if (flag == kSetupFlag) {
    OnTimingSetup(timing);
  } else if (flag == kUpdateFlag || flag == kForceUpdateFlag) {
    OnTimingUpdate(timing, flag);
  }
}

void TimingClientDelegate::OnTimingSetup(
    const std::unordered_map<std::string, int64_t>& timing) {
  if (platform_view_) {
    platform_view_->OnTimingSetup(timing);
  }
}

void TimingClientDelegate::OnTimingUpdate(
    const std::unordered_map<std::string, int64_t>& timing,
    const std::string& flag) {
  if (platform_view_) {
    platform_view_->OnTimingUpdate(timing, flag);
  }
}

}  // namespace clay
