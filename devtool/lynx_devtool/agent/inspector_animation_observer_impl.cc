// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_animation_observer_impl.h"

#include <utility>

#include "core/animation/animation.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorAnimationObserverImpl::InspectorAnimationObserverImpl(
    const std::shared_ptr<InspectorTasmExecutor>& animation_executor,
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : animation_executor_wp_(animation_executor),
      devtool_mediator_wp_(devtool_mediator) {}

void InspectorAnimationObserverImpl::OnAnimationCreated(
    animation::Animation* animation) {
  auto mediator = devtool_mediator_wp_.lock();
  if (!animation || !mediator || animation_executor_wp_.expired()) {
    return;
  }
  mediator->RunOnTASMThread(
      [executor_wp = animation_executor_wp_, id = animation->id(),
       animation_wp = animation->weak_from_this()]() mutable {
        if (auto executor = executor_wp.lock()) {
          executor->OnAnimationCreated(id, std::move(animation_wp));
        }
      },
      false);
}

void InspectorAnimationObserverImpl::OnAnimationStarted(
    animation::Animation* animation) {
  auto mediator = devtool_mediator_wp_.lock();
  if (!animation || !mediator || animation_executor_wp_.expired()) {
    return;
  }
  auto snapshot =
      InspectorAnimationController::BuildAnimationSnapshot(animation);
  mediator->RunOnTASMThread(
      [executor_wp = animation_executor_wp_, snapshot = std::move(snapshot)] {
        if (auto executor = executor_wp.lock()) {
          executor->OnAnimationStarted(snapshot);
        }
      },
      false);
}

void InspectorAnimationObserverImpl::OnAnimationUpdated(
    animation::Animation* animation) {
  auto mediator = devtool_mediator_wp_.lock();
  if (!animation || !mediator || animation_executor_wp_.expired()) {
    return;
  }
  auto snapshot =
      InspectorAnimationController::BuildAnimationSnapshot(animation);
  mediator->RunOnTASMThread(
      [executor_wp = animation_executor_wp_, snapshot = std::move(snapshot)] {
        if (auto executor = executor_wp.lock()) {
          executor->OnAnimationUpdated(snapshot);
        }
      },
      false);
}

void InspectorAnimationObserverImpl::OnAnimationCanceled(
    animation::Animation* animation) {
  auto mediator = devtool_mediator_wp_.lock();
  if (!animation || !mediator || animation_executor_wp_.expired()) {
    return;
  }
  mediator->RunOnTASMThread(
      [executor_wp = animation_executor_wp_, id = animation->id()] {
        if (auto executor = executor_wp.lock()) {
          executor->OnAnimationCanceled(id);
        }
      },
      false);
}

}  // namespace devtool
}  // namespace lynx
