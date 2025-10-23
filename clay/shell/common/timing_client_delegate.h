// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_TIMING_CLIENT_DELEGATE_H_
#define CLAY_SHELL_COMMON_TIMING_CLIENT_DELEGATE_H_

#include <string>
#include <unordered_map>

#include "clay/shell/common/platform_view.h"

namespace clay {

// Must be used on Platform Thread.
class TimingClientDelegate {
 public:
  explicit TimingClientDelegate(clay::PlatformView* platform_view)
      : platform_view_(platform_view) {}

  void ReportTiming(const std::unordered_map<std::string, int64_t>& timing,
                    const std::string& flag);

  void OnTimingSetup(const std::unordered_map<std::string, int64_t>& timing);
  void OnTimingUpdate(const std::unordered_map<std::string, int64_t>& timing,
                      const std::string& flag);

 private:
  clay::PlatformView* platform_view_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_TIMING_CLIENT_DELEGATE_H_
