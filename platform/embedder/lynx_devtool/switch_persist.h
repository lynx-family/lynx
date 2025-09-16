// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_DEVTOOL_SWITCH_PERSIST_H_
#define PLATFORM_EMBEDDER_LYNX_DEVTOOL_SWITCH_PERSIST_H_

#include <string>

namespace lynx {
namespace embedder {
struct SwitchPersist {
  static bool GetValueFromPersistent(const std::string& key,
                                     bool default_value);
  static bool SetValueToPersistent(const std::string& key, bool value);
};
}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_DEVTOOL_SWITCH_PERSIST_H_
