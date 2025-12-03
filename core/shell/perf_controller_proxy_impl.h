// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_PERF_CONTROLLER_PROXY_IMPL_H_
#define CORE_SHELL_PERF_CONTROLLER_PROXY_IMPL_H_

#include <memory>
#include <string>

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

  void SetHostPlatformType(const std::string& type) override;

  void MarkTiming(tasm::TimingKey timing_key,
                  const tasm::PipelineID& pipeline_id) override;
  void SetTiming(tasm::TimingKey timing_key, uint64_t timestamp_us,
                 const tasm::PipelineID& pipeline_id) override;
  void SetHostPlatformTiming(tasm::TimingKey timing_key, uint64_t timestamp_us,
                             const tasm::PipelineID& pipeline_id) override;
  std::string GetPlatform() const override;

  void RunTaskInReportThread(base::closure task) override;

  void OnEvent(int32_t instance_id, ReportEvent& event) override;

 protected:
  std::shared_ptr<shell::LynxActor<tasm::performance::PerformanceController>>
      perf_actor_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_PERF_CONTROLLER_PROXY_IMPL_H_
