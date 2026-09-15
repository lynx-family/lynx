// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_animation_observer_impl.h"

namespace lynx {
namespace devtool {

InspectorAnimationObserverImpl::InspectorAnimationObserverImpl(
    const std::shared_ptr<InspectorTasmExecutor>& animation_executor)
    : animation_executor_wp_(animation_executor) {}

void InspectorAnimationObserverImpl::OnAnimationCreated(
    lynx::animation::Animation* animation) {
  auto executor = animation_executor_wp_.lock();
  if (executor == nullptr) {
    return;
  }
  executor->OnAnimationCreated(animation);
}

void InspectorAnimationObserverImpl::OnAnimationStarted(
    lynx::animation::Animation* animation) {
  auto executor = animation_executor_wp_.lock();
  if (executor == nullptr) {
    return;
  }
  executor->OnAnimationStarted(animation);
}

void InspectorAnimationObserverImpl::OnAnimationUpdated(
    lynx::animation::Animation* animation) {
  auto executor = animation_executor_wp_.lock();
  if (executor == nullptr) {
    return;
  }
  executor->OnAnimationUpdated(animation);
}

void InspectorAnimationObserverImpl::OnAnimationCanceled(
    lynx::animation::Animation* animation) {
  auto executor = animation_executor_wp_.lock();
  if (executor == nullptr) {
    return;
  }
  executor->OnAnimationCanceled(animation);
}

}  // namespace devtool
}  // namespace lynx
