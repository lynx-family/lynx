// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_HARMONY_PERFORMANCE_CONTROLLER_HARMONY_H_
#define CORE_SERVICES_PERFORMANCE_HARMONY_PERFORMANCE_CONTROLLER_HARMONY_H_

#include <node_api.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "core/public/performance_controller_platform_impl.h"
#include "core/services/performance/performance_controller.h"

namespace lynx {
namespace tasm {
namespace performance {

class PerformanceControllerHarmonyJSWrapper
    : public std::enable_shared_from_this<
          PerformanceControllerHarmonyJSWrapper> {
 public:
  PerformanceControllerHarmonyJSWrapper(napi_env env) : env_(env){};

  void OnPerformanceEvent(const std::unique_ptr<pub::Value>& entry);

  ~PerformanceControllerHarmonyJSWrapper();

  void SetActor(
      const std::shared_ptr<shell::LynxActor<PerformanceController>>& actor) {
    actor_ = actor;
  }
  const std::weak_ptr<shell::LynxActor<PerformanceController>>& GetActor() {
    return actor_;
  }
  static napi_value Init(napi_env env, napi_value exports);

  PerformanceControllerHarmonyJSWrapper(
      const PerformanceControllerHarmonyJSWrapper&) = delete;
  PerformanceControllerHarmonyJSWrapper& operator=(
      const PerformanceControllerHarmonyJSWrapper&) = delete;
  PerformanceControllerHarmonyJSWrapper(
      PerformanceControllerHarmonyJSWrapper&&) = delete;
  PerformanceControllerHarmonyJSWrapper& operator=(
      PerformanceControllerHarmonyJSWrapper&&) = delete;

 private:
  static napi_value Constructor(napi_env env, napi_callback_info info);
  static napi_value SetTiming(napi_env env, napi_callback_info info);
  static napi_value MarkPaintEndTimingIfNeeded(napi_env env,
                                               napi_callback_info info);
  napi_ref js_impl_strong_ref_{nullptr};
  napi_ref js_on_performance_event_func_ref_{nullptr};
  napi_env env_{nullptr};
  std::weak_ptr<shell::LynxActor<PerformanceController>> actor_;
};

class PerformanceControllerHarmony : public PerformanceControllerPlatformImpl {
 public:
  PerformanceControllerHarmony(
      std::shared_ptr<PerformanceControllerHarmonyJSWrapper> wrapper)
      : js_wrapper_(std::move(wrapper)){};

  ~PerformanceControllerHarmony() override = default;

  void OnPerformanceEvent(
      const std::unique_ptr<pub::Value>& entry_map) override {
    if (js_wrapper_) {
      js_wrapper_->OnPerformanceEvent(entry_map);
    }
  }

  void SetActor(const std::shared_ptr<shell::LynxActor<PerformanceController>>&
                    actor) override {
    js_wrapper_->SetActor(actor);
  }

  PerformanceControllerHarmony(const PerformanceControllerHarmony&) = delete;
  PerformanceControllerHarmony& operator=(const PerformanceControllerHarmony&) =
      delete;
  PerformanceControllerHarmony(PerformanceControllerHarmony&&) = delete;
  PerformanceControllerHarmony& operator=(PerformanceControllerHarmony&&) =
      delete;

 private:
  std::shared_ptr<PerformanceControllerHarmonyJSWrapper> js_wrapper_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_HARMONY_PERFORMANCE_CONTROLLER_HARMONY_H_
