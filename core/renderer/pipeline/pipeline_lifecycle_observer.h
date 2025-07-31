// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_PIPELINE_PIPELINE_LIFECYCLE_OBSERVER_H_
#define CORE_RENDERER_PIPELINE_PIPELINE_LIFECYCLE_OBSERVER_H_

#include <string>

#include "base/include/timer/time_utils.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/pipeline/pipeline_lifecycle.h"
#include "core/renderer/pipeline/pipeline_version.h"

namespace lynx {
namespace tasm {
class PipelineLifecycleObserver {
 public:
  class Data {
   public:
    Data()
        : pipeline_version(PipelineVersion::Create()),
          pipeline_id(std::string("")),
          timestamp_us(base::CurrentSystemTimeMicroseconds()) {}
    ~Data() = default;

    LifecycleState prev_state = LifecycleState::kUninitialized;
    LifecycleState cur_state = LifecycleState::kUninitialized;
    bool is_state_executed = false;
    PipelineVersion pipeline_version;
    PipelineID pipeline_id;
    PipelineOrigin pipeline_origin;
    uint64_t timestamp_us;
  };
  PipelineLifecycleObserver() = default;
  virtual ~PipelineLifecycleObserver() = default;
  PipelineLifecycleObserver(const PipelineLifecycleObserver&) = delete;
  PipelineLifecycleObserver& operator=(const PipelineLifecycleObserver&) =
      delete;
  PipelineLifecycleObserver(PipelineLifecycleObserver&&) = default;
  PipelineLifecycleObserver& operator=(PipelineLifecycleObserver&&) = default;

  virtual void OnLifecycleChanged(const Data& lifecycle_data) = 0;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_PIPELINE_PIPELINE_LIFECYCLE_OBSERVER_H_
