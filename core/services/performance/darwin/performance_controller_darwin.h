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
#include "core/services/performance/performance_controller.h"

namespace lynx {
namespace tasm {
namespace performance {

class PerformanceControllerDarwin : public PerformanceController {
 public:
  explicit PerformanceControllerDarwin(
      LynxPerformanceController* platform_performance_controller)
      : platform_performance_controller_(platform_performance_controller){};

  ~PerformanceControllerDarwin() override = default;

  void OnPerformanceEvent(
      const std::unique_ptr<lynx::pub::Value> entry) override;

  void SetActor(const std::shared_ptr<lynx::shell::LynxActor<
                    lynx::tasm::performance::PerformanceController>>& actor);

  PerformanceControllerDarwin(const PerformanceControllerDarwin&) = delete;
  PerformanceControllerDarwin& operator=(const PerformanceControllerDarwin&) =
      delete;
  PerformanceControllerDarwin(PerformanceControllerDarwin&&) = default;
  PerformanceControllerDarwin& operator=(PerformanceControllerDarwin&&) =
      default;

 private:
  __weak LynxPerformanceController* platform_performance_controller_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_DARWIN_PERFORMANCE_CONTROLLER_DARWIN_H_
