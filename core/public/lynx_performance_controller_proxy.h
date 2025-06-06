// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_LYNX_PERFORMANCE_CONTROLLER_PROXY_H_
#define CORE_PUBLIC_LYNX_PERFORMANCE_CONTROLLER_PROXY_H_
#include <memory>
namespace lynx {
namespace shell {
class LynxPerformanceControllerProxy {
 public:
  virtual ~LynxPerformanceControllerProxy() = default;
  virtual void MarkPaintEndTimingIfNeeded() = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_PUBLIC_LYNX_PERFORMANCE_CONTROLLER_PROXY_H_
