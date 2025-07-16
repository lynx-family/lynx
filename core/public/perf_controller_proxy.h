// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_PERF_CONTROLLER_PROXY_H_
#define CORE_PUBLIC_PERF_CONTROLLER_PROXY_H_
#include <memory>

#include "core/public/pipeline_option.h"

namespace lynx {
namespace shell {
class PerfControllerProxy {
 public:
  virtual ~PerfControllerProxy() = default;
  virtual void MarkPaintEndTimingIfNeeded() = 0;
  virtual void SetNeedMarkPaintEndTiming(
      const tasm::PipelineID& pipeline_id) = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_PUBLIC_PERF_CONTROLLER_PROXY_H_
