// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_PERFORMANCE_CONTROLLER_PROXY_IMPL_H_
#define CORE_SHELL_LYNX_PERFORMANCE_CONTROLLER_PROXY_IMPL_H_

#include <memory>

#include "core/public/lynx_performance_controller_proxy.h"
#include "core/services/performance/performance_controller.h"

namespace lynx {
namespace shell {
class LynxPerformanceControllerProxyImpl
    : public LynxPerformanceControllerProxy {
 public:
  LynxPerformanceControllerProxyImpl(
      std::shared_ptr<
          shell::LynxActor<tasm::performance::PerformanceController>>
          actor)
      : perf_actor_(actor) {}
  ~LynxPerformanceControllerProxyImpl() = default;
  void MarkPaintEndTimingIfNeeded() override;

 protected:
  std::shared_ptr<shell::LynxActor<tasm::performance::PerformanceController>>
      perf_actor_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_PERFORMANCE_CONTROLLER_PROXY_IMPL_H_
