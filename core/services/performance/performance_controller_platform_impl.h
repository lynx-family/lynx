// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_PLATFORM_IMPL_H_
#define CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_PLATFORM_IMPL_H_

#include <memory>

#include "core/public/performance_controller_platform.h"
#include "core/services/performance/performance_controller.h"

namespace lynx {
namespace tasm {
namespace performance {

// @class PerformanceController
// @brief Base class for performance monitoring system
// Integrates memory monitoring with performance reporting functionality.
// Serves as the delegate for MemoryMonitor and provides common infrastructure.
class PerformanceControllerPlatformImpl
    : public PerformanceControllerPlatform<PerformanceController> {
 public:
  PerformanceControllerPlatformImpl() = default;
  ~PerformanceControllerPlatformImpl() override;

  void OnPerformanceEvent(const std::unique_ptr<pub::Value>& entry) override;

  void SetActor(const std::shared_ptr<shell::LynxActor<PerformanceController>>&
                    actor) override;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_PLATFORM_IMPL_H_
