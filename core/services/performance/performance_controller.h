// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_H_
#define CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_H_

#include <cstddef>
#include <memory>
#include <utility>

#include "base/include/lynx_actor.h"
#include "core/public/performance_controller_platform.h"
#include "core/public/pub_value.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "core/services/performance/performance_entry_sender.h"
#include "core/services/timing_handler/timing_handler.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {
namespace performance {

// @class PerformanceController
// @brief Base class for performance monitoring system
// Integrates memory monitoring with performance reporting functionality.
// Serves as the delegate for MemoryMonitor and provides common infrastructure.
class PerformanceController : public PerformanceEntrySender {
 public:
  PerformanceController(
      std::unique_ptr<PerformanceEntrySender> delegate,
      std::unique_ptr<timing::TimingHandlerDelegate> timing_delegate,
      int32_t instance_id)
      : PerformanceEntrySender(std::make_shared<pub::PubValueFactoryDefault>()),
        instance_id_(instance_id),
        delegate_(std::move(delegate)),
        memory_monitor_(MemoryMonitor(this)),
        timing_handler_(
            timing::TimingHandler(this, std::move(timing_delegate))) {}
  virtual ~PerformanceController() override = default;

  static fml::RefPtr<fml::TaskRunner> GetTaskRunner();

  void SetPlatformImpl(
      std::unique_ptr<PerformanceControllerPlatform<PerformanceController>>
          platform_impl);

  const std::unique_ptr<PerformanceControllerPlatform<PerformanceController>>&
  GetPlatformImpl() {
    return platform_impl_;
  }

  void OnPerformanceEvent(
      std::unique_ptr<pub::Value> entry,
      PerformanceObserverEnv env = kPerformanceObserverOfAll) override;

  const std::shared_ptr<pub::PubValueFactory>& GetValueFactory() override {
    return value_factory_;
  }

  MemoryMonitor& GetMemoryMonitor() { return memory_monitor_; }
  timing::TimingHandler& GetTimingHandler() { return timing_handler_; }

  void SetInstanceId(int32_t instance_id) { instance_id_ = instance_id; }

  PerformanceController(const PerformanceController&) = delete;
  PerformanceController& operator=(const PerformanceController&) = delete;
  PerformanceController(PerformanceController&&) = delete;
  PerformanceController& operator=(PerformanceController&&) = delete;

 private:
  int32_t instance_id_ = report::kUninitializedInstanceId;
  std::unique_ptr<PerformanceEntrySender> delegate_;
  std::unique_ptr<PerformanceControllerPlatform<PerformanceController>>
      platform_impl_;
  MemoryMonitor memory_monitor_;
  timing::TimingHandler timing_handler_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_H_
