// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_PERF_CONTROLLER_PROXY_IMPL_H_
#define CORE_SHELL_PERF_CONTROLLER_PROXY_IMPL_H_

#include <memory>

#include "core/public/perf_controller_proxy.h"
#include "core/public/performance_controller_platform_impl.h"
#include "core/services/performance/performance_controller.h"

namespace lynx {
namespace shell {
class PerfControllerProxyImpl : public PerfControllerProxy {
 public:
  PerfControllerProxyImpl(
      std::shared_ptr<
          shell::LynxActor<tasm::performance::PerformanceController>>
          actor)
      : perf_actor_(actor) {}
  ~PerfControllerProxyImpl() = default;

  void MarkTiming(tasm::TimingKey timing_key,
                  const tasm::PipelineID& pipeline_id) override;

 protected:
  std::shared_ptr<shell::LynxActor<tasm::performance::PerformanceController>>
      perf_actor_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_PERF_CONTROLLER_PROXY_IMPL_H_
