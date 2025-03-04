// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_TIMING_COLLECTOR_PLATFORM_H_
#define CORE_PUBLIC_TIMING_COLLECTOR_PLATFORM_H_

#include <string>

#include "core/public/pipeline_option.h"

namespace lynx {
namespace shell {

class TimingCollectorPlatform {
 public:
  virtual ~TimingCollectorPlatform() = default;

  virtual void MarkTiming(const tasm::PipelineID& pipeline_id,
                          const std::string& timing_key) const = 0;

  virtual void SetTiming(const tasm::PipelineID& pipeline_id,
                         const std::string& timing_key,
                         uint64_t us_timestamp) const = 0;

  virtual void SetNeedMarkDrawEndTiming(
      const tasm::PipelineID& pipeline_id) = 0;

  virtual void MarkDrawEndTimingIfNeeded() = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_PUBLIC_TIMING_COLLECTOR_PLATFORM_H_
