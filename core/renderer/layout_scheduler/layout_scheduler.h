// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_LAYOUT_SCHEDULER_LAYOUT_SCHEDULER_H_
#define CORE_RENDERER_LAYOUT_SCHEDULER_LAYOUT_SCHEDULER_H_

#include <memory>

#include "core/public/pipeline_option.h"

namespace lynx {
namespace tasm {

class LayoutScheduler {
 public:
  class LayoutSchedulerImpl {
   public:
    LayoutSchedulerImpl() = default;
    virtual ~LayoutSchedulerImpl() = default;
    virtual void RequestLayout(
        const std::shared_ptr<tasm::PipelineOptions>& options) = 0;
  };

  explicit LayoutScheduler(LayoutSchedulerImpl* impl);
  ~LayoutScheduler() = default;

  void RequestLayout(const std::shared_ptr<tasm::PipelineOptions>& options);

 private:
  LayoutSchedulerImpl* impl_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_LAYOUT_SCHEDULER_LAYOUT_SCHEDULER_H_
