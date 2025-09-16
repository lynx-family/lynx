// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_CORE_PERFORMANCE_PERFORMANCE_CONTROLLER_IMPL_H_
#define PLATFORM_EMBEDDER_CORE_PERFORMANCE_PERFORMANCE_CONTROLLER_IMPL_H_

#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "core/public/performance_controller_platform_impl.h"
#include "core/services/performance/performance_controller.h"
#include "platform/embedder/core/lynx_template_renderer.h"

namespace lynx {
namespace embedder {

class PerformanceControllerImpl
    : public tasm::performance::PerformanceControllerPlatformImpl {
 public:
  explicit PerformanceControllerImpl(
      std::weak_ptr<LynxTemplateRenderer::WeakFlag> weak_renderer)
      : weak_renderer_(weak_renderer) {}

  ~PerformanceControllerImpl() override = default;

  void OnPerformanceEvent(const std::unique_ptr<pub::Value>& entry) override;

  void SetActor(const std::shared_ptr<
                shell::LynxActor<tasm::performance::PerformanceController>>&
                    actor) override {
    perf_controller_weak_ptr_ = actor;
  }

  PerformanceControllerImpl(const PerformanceControllerImpl&) = delete;
  PerformanceControllerImpl& operator=(const PerformanceControllerImpl&) =
      delete;
  PerformanceControllerImpl(PerformanceControllerImpl&&) = delete;
  PerformanceControllerImpl& operator=(PerformanceControllerImpl&&) = delete;

  lepus::Value PreparePerformanceData(
      const std::unique_ptr<pub::Value>& entry_map);
  void SendPerformanceData(const lepus::Value& lepus_entry_map);

 private:
  std::weak_ptr<LynxTemplateRenderer::WeakFlag> weak_renderer_;
  std::weak_ptr<shell::LynxActor<tasm::performance::PerformanceController>>
      perf_controller_weak_ptr_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_CORE_PERFORMANCE_PERFORMANCE_CONTROLLER_IMPL_H_
