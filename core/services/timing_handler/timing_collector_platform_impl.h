// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_TIMING_HANDLER_TIMING_COLLECTOR_PLATFORM_IMPL_H_
#define CORE_SERVICES_TIMING_HANDLER_TIMING_COLLECTOR_PLATFORM_IMPL_H_

#include <memory>
#include <string>

#include "core/public/timing_collector_platform.h"
#include "core/services/timing_handler/timing_handler.h"
#include "core/shell/lynx_actor_specialization.h"

namespace lynx {
namespace tasm {
namespace timing {

class TimingCollectorPlatformImpl : public shell::TimingCollectorPlatform {
 public:
  TimingCollectorPlatformImpl() = default;
  ~TimingCollectorPlatformImpl() = default;

  void MarkTiming(const tasm::PipelineID& pipeline_id,
                  const std::string& timing_key) const override;

  void SetTiming(const tasm::PipelineID& pipeline_id,
                 const std::string& timing_key,
                 uint64_t us_timestamp) const override;

  void SetNeedMarkDrawEndTiming(const tasm::PipelineID& pipeline_id) override;

  void MarkDrawEndTimingIfNeeded() override;

  inline void SetTimingActor(
      const std::shared_ptr<shell::LynxActor<tasm::timing::TimingHandler>>&
          timing_actor) {
    timing_actor_ = timing_actor;
  };

 protected:
  std::shared_ptr<shell::LynxActor<TimingHandler>>
      timing_actor_;  // on timing runner

 private:
  base::ConcurrentQueue<tasm::PipelineID> paint_end_pipeline_id_list_;
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_COLLECTOR_PLATFORM_IMPL_H_
