// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_GLOBAL_DEVTOOL_PLATFORM_EMBEDDER_H_
#define DEVTOOL_EMBEDDER_CORE_GLOBAL_DEVTOOL_PLATFORM_EMBEDDER_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/include/closure.h"

namespace lynx {
namespace devtool {

class GlobalDevtoolPlatformEmbedder
    : public std::enable_shared_from_this<GlobalDevtoolPlatformEmbedder> {
 public:
  GlobalDevtoolPlatformEmbedder() = default;
  ~GlobalDevtoolPlatformEmbedder() = default;

  static void StartMemoryTracing();
  static void StopMemoryTracing();
  static void GetAllMemoryUsage(
      int64_t timeout_ms,
      base::MoveOnlyClosure<void, const std::string&, const std::string&>
          callback);
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_GLOBAL_DEVTOOL_PLATFORM_EMBEDDER_H_
