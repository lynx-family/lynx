// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_DARWIN_PERFORMANCE_CONTROLLER_DARWIN_H_
#define CORE_SERVICES_PERFORMANCE_DARWIN_PERFORMANCE_CONTROLLER_DARWIN_H_

#import <Lynx/LynxPerformanceObserverProtocol.h>

#include <memory>
#include <string>
#include <utility>

#import "LynxPerformanceController.h"
#include "core/public/performance_controller_platform.h"
#include "core/services/performance/performance_controller.h"

namespace lynx {
namespace tasm {
namespace performance {

class PerformanceControllerDarwin
    : public PerformanceControllerPlatform<PerformanceController> {
 public:
  explicit PerformanceControllerDarwin(
      LynxPerformanceController* platform_performance_controller)
      : platform_performance_controller_(platform_performance_controller){};

  void SetActor(const std::shared_ptr<shell::LynxActor<PerformanceController>>&
                    actor) override;
  void OnPerformanceEvent(const std::unique_ptr<pub::Value>& entry) override;

  PerformanceControllerDarwin(const PerformanceControllerDarwin&) = delete;
  PerformanceControllerDarwin& operator=(const PerformanceControllerDarwin&) =
      delete;
  PerformanceControllerDarwin(PerformanceControllerDarwin&&) = default;
  PerformanceControllerDarwin& operator=(PerformanceControllerDarwin&&) =
      default;

 private:
  __strong LynxPerformanceController* platform_performance_controller_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_DARWIN_PERFORMANCE_CONTROLLER_DARWIN_H_
