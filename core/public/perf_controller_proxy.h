// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_PERF_CONTROLLER_PROXY_H_
#define CORE_PUBLIC_PERF_CONTROLLER_PROXY_H_
#include <memory>

#include "core/public/pipeline_option.h"
#include "core/public/timing_key.h"

namespace lynx {
namespace shell {
class PerfControllerProxy {
 public:
  virtual ~PerfControllerProxy() = default;

  /**
   * @brief Marks a timing event with the specified key and pipeline ID.
   * @param timing_key The key that uniquely identifies the timing event.
   * @param pipeline_id The optional pipeline ID associated with the timing
   * event.
   */
  virtual void MarkTiming(tasm::TimingKey timing_key,
                          const tasm::PipelineID& pipeline_id) = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_PUBLIC_PERF_CONTROLLER_PROXY_H_
