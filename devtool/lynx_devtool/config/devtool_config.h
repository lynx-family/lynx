// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_CONFIG_DEVTOOL_CONFIG_H_
#define DEVTOOL_LYNX_DEVTOOL_CONFIG_DEVTOOL_CONFIG_H_

#include <atomic>

namespace lynx {
namespace devtool {

class DevToolConfig {
 public:
  static void SetStopAtEntry(bool stop_at_entry, bool is_lepus = false);
  static bool ShouldStopAtEntry(bool is_lepus = false);

 private:
  static std::atomic<bool> should_stop_at_entry_;
  static std::atomic<bool> should_stop_lepus_at_entry_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_CONFIG_DEVTOOL_CONFIG_H_
