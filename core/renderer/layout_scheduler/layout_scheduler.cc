// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/layout_scheduler/layout_scheduler.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace tasm {

LayoutScheduler::LayoutScheduler(LayoutSchedulerImpl* impl) : impl_(impl) {}

void LayoutScheduler::RequestLayout(
    const std::shared_ptr<tasm::PipelineOptions>& options) {
  if (impl_ == nullptr) {
    LOGE("LayoutScheduler::RequestLayout failed since the impl_ is nullptr.");
    return;
  }
  impl_->RequestLayout(options);
}

}  // namespace tasm
}  // namespace lynx
