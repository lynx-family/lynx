// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_H_
#define CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_H_

#include <cstddef>
#include <memory>

#include "base/include/lynx_actor.h"
#include "core/public/pub_value.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {
namespace performance {

// @class PerformanceController
// @brief Base class for performance monitoring system
// Integrates memory monitoring with performance reporting functionality.
// Serves as the delegate for MemoryMonitor and provides common infrastructure.
class PerformanceController : public MemoryMonitor::Delegate {
 public:
  PerformanceController()
      : instance_id_(report::kUninitializedInstanceId),
        value_factory_(std::make_unique<pub::PubValueFactoryDefault>()),
        memory_monitor_(MemoryMonitor(this)) {}
  virtual ~PerformanceController() override = default;

  void SetActor(
      const std::shared_ptr<shell::LynxActor<PerformanceController>>& actor);

  void OnPerformanceEvent(const std::unique_ptr<pub::Value> entry) override;

  const std::unique_ptr<pub::PubValueFactory>& GetValueFactory() override {
    return value_factory_;
  }

  MemoryMonitor& GetMemoryMonitor() { return memory_monitor_; }

  void SetInstanceId(int32_t instance_id) { instance_id_ = instance_id; }

  int32_t GetInstanceId() override { return instance_id_; }

  PerformanceController(const PerformanceController&) = delete;
  PerformanceController& operator=(const PerformanceController&) = delete;
  PerformanceController(PerformanceController&&);
  PerformanceController& operator=(PerformanceController&&);

 private:
  int32_t instance_id_ = report::kUninitializedInstanceId;
  std::unique_ptr<pub::PubValueFactory> value_factory_;
  MemoryMonitor memory_monitor_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_H_
