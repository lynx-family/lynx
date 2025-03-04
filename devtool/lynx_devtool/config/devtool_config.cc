// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/config/devtool_config.h"

namespace lynx {
namespace devtool {

std::atomic<bool> DevToolConfig::should_stop_at_entry_ = {false};
std::atomic<bool> DevToolConfig::should_stop_lepus_at_entry_ = {false};

void DevToolConfig::SetStopAtEntry(bool stop_at_entry, bool is_lepus) {
  if (is_lepus) {
    should_stop_lepus_at_entry_ = stop_at_entry;
  } else {
    should_stop_at_entry_ = stop_at_entry;
  }
}

bool DevToolConfig::ShouldStopAtEntry(bool is_lepus) {
  if (is_lepus) {
    return should_stop_lepus_at_entry_;
  }
  return should_stop_at_entry_;
}

}  // namespace devtool
}  // namespace lynx
