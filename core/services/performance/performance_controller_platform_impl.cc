// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/performance_controller_platform_impl.h"

namespace lynx {
namespace tasm {
namespace performance {

void PerformanceControllerPlatformImpl::OnPerformanceEvent(
    const std::unique_ptr<pub::Value>& entry) override {
  // TODO: Implement Darwin & Android platform support later.
}

void PerformanceControllerPlatformImpl::SetActor(
    const std::shared_ptr<shell::LynxActor<PerformanceController>>& actor)
    override {
  // TODO: Implement Darwin & Android platform support later.
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
