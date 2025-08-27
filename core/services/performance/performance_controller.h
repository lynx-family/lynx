// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_H_
#define CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_H_

#include <cstddef>
#include <memory>
#include <utility>

#include "base/include/lynx_actor.h"
#include "core/public/pub_value.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/performance/js_blocking_monitor/js_blocking_monitor.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "core/services/performance/performance_event_sender.h"
#include "core/services/timing_handler/timing_handler.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {
namespace performance {
class PerformanceControllerPlatformImpl;

// @class PerformanceController
// @brief Base class for performance monitoring system
// Integrates memory monitoring with performance reporting functionality.
// Serves as the delegate for MemoryMonitor and provides common infrastructure.
class PerformanceController : public PerformanceEventSender {
 public:
  PerformanceController(
      std::unique_ptr<PerformanceEventSender> delegate,
      std::unique_ptr<timing::TimingHandlerDelegate> timing_delegate,
      int32_t instance_id)
      : PerformanceEventSender(std::make_shared<pub::PubValueFactoryDefault>()),
        instance_id_(instance_id),
        delegate_(std::move(delegate)),
        memory_monitor_(this, instance_id),
        timing_handler_(
            timing::TimingHandler(std::move(timing_delegate), this)),
        js_blocking_monitor_(std::make_shared<JSBlockingMonitor>(this)) {}
  ~PerformanceController() override;

  static fml::RefPtr<fml::TaskRunner> GetTaskRunner();

  void SetPlatformImpl(
      std::unique_ptr<PerformanceControllerPlatformImpl> platform_impl);

  const std::unique_ptr<PerformanceControllerPlatformImpl>& GetPlatformImpl() {
    return platform_impl_;
  }

  void OnPerformanceEvent(std::unique_ptr<pub::Value> entry,
                          EventType type = kEventTypeAll) override;

  const std::shared_ptr<pub::PubValueFactory>& GetValueFactory() override {
    return value_factory_;
  }

  inline void SetEnableMainThreadCallback(bool enable) override {
    enable_main_thread_engine_callback_ = enable;
    if (delegate_) {
      delegate_->SetEnableMainThreadCallback(enable);
    }
    timing_handler_.SetEnableAirStrictMode(enable);
  };

  MemoryMonitor& GetMemoryMonitor() { return memory_monitor_; }
  timing::TimingHandler& GetTimingHandler() { return timing_handler_; }
  std::shared_ptr<JSBlockingMonitor>& GetJSBlockingMonitor() {
    return js_blocking_monitor_;
  }

  void SetInstanceId(int32_t instance_id) { instance_id_ = instance_id; }

  int32_t GetInstanceId() { return instance_id_; }

  PerformanceController(const PerformanceController&) = delete;
  PerformanceController& operator=(const PerformanceController&) = delete;
  PerformanceController(PerformanceController&&) = delete;
  PerformanceController& operator=(PerformanceController&&) = delete;

 private:
  int32_t instance_id_ = report::kUninitializedInstanceId;
  std::unique_ptr<PerformanceEventSender> delegate_;
  std::unique_ptr<PerformanceControllerPlatformImpl> platform_impl_;
  MemoryMonitor memory_monitor_;
  timing::TimingHandler timing_handler_;
  std::shared_ptr<JSBlockingMonitor> js_blocking_monitor_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_PERFORMANCE_CONTROLLER_H_
