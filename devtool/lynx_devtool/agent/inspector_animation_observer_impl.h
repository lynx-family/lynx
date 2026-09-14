// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_ANIMATION_OBSERVER_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_ANIMATION_OBSERVER_IMPL_H_

#include <memory>

#include "core/inspector/observer/inspector_animation_observer.h"
#include "devtool/lynx_devtool/agent/inspector_tasm_executor.h"

namespace lynx {
namespace devtool {

// Concrete InspectorAnimationObserver that forwards animation lifecycle
// callbacks to the InspectorTasmExecutor (which runs on the TASM thread, the
// same thread the animations live on). Mirrors InspectorElementObserverImpl.
class InspectorAnimationObserverImpl
    : public lynx::tasm::InspectorAnimationObserver {
 public:
  explicit InspectorAnimationObserverImpl(
      const std::shared_ptr<InspectorTasmExecutor>& animation_executor);
  ~InspectorAnimationObserverImpl() override = default;

  void OnAnimationCreated(lynx::animation::Animation* animation) override;
  void OnAnimationStarted(lynx::animation::Animation* animation) override;
  void OnAnimationUpdated(lynx::animation::Animation* animation) override;
  void OnAnimationCanceled(lynx::animation::Animation* animation) override;

 private:
  std::weak_ptr<InspectorTasmExecutor> animation_executor_wp_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_ANIMATION_OBSERVER_IMPL_H_
