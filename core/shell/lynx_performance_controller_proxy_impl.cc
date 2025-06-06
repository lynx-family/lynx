// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/lynx_performance_controller_proxy_impl.h"

namespace lynx {
namespace shell {
void LynxPerformanceControllerProxyImpl::MarkPaintEndTimingIfNeeded() {
  perf_actor_->ActAsync([timestamp = base::CurrentSystemTimeMicroseconds()](
                            auto& controller) mutable {
    controller->GetTimingHandler().SetPaintEndTimingIfNeeded(
        static_cast<lynx::tasm::timing::TimestampUs>(timestamp));
  });
}
}  // namespace shell
}  // namespace lynx
