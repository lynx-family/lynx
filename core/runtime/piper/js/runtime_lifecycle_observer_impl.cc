// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/piper/js/runtime_lifecycle_observer_impl.h"

#include <utility>

#include "base/include/fml/task_runner.h"
#include "base/include/log/logging.h"
#include "core/runtime/piper/js/runtime_lifecycle_listener_delegate.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/napi.h"
#include "third_party/napi/include/primjs_napi_defines.h"
#else
#include "third_party/binding/napi/shim/shim_napi.h"
#endif

namespace lynx {
namespace runtime {
namespace {
constexpr int PART_DELEGATE_FLAG =
    ~(RuntimeLifecycleObserverImpl::LifecycleState::ATTACH |
      RuntimeLifecycleObserverImpl::LifecycleState::DETACH);
constexpr int FULL_DELEGATE_FLAG = 0;
}  // namespace

void RuntimeLifecycleObserverImpl::OnRuntimeCreate(
    std::shared_ptr<runtime::IVSyncObserver> observer) {
  LOGI("[Runtime] RuntimeLifecycleObserverImpl::OnRuntimeCreate with observer:"
       << observer);
  auto op = LifecycleState::CREATE;
  event_record_.emplace_back(op);
  args_vsync_observer_ = observer;
  for (auto& pair : delegates_) {
    if (!(pair.second & op)) {
      pair.first->OnRuntimeCreate(observer);
      pair.second |= op;
    }
  }
}

void RuntimeLifecycleObserverImpl::OnRuntimeInit(int64_t runtime_id) {
  LOGI(
      "[Runtime] RuntimeLifecycleObserverImpl::OnRuntimeInit with runtime "
      "id: "
      << runtime_id);
  auto op = LifecycleState::INIT;
  event_record_.emplace_back(op);
  args_runtime_id_ = runtime_id;
  for (auto& pair : delegates_) {
    if (!(pair.second & op)) {
      pair.first->OnRuntimeInit(runtime_id);
      pair.second |= op;
    }
  }
}

void RuntimeLifecycleObserverImpl::OnAppEnterForeground() {
  LOGI("[Runtime] RuntimeLifecycleObserverImpl::OnAppEnterForeground");
  auto op = LifecycleState::ENTER_FOREGROUND;
  event_record_.emplace_back(op);
  for (auto& pair : delegates_) {
    if (!(pair.second & op)) {
      pair.first->OnAppEnterForeground();
    }
  }
}
void RuntimeLifecycleObserverImpl::OnAppEnterBackground() {
  LOGI("[Runtime] RuntimeLifecycleObserverImpl::OnAppEnterBackground");
  auto op = LifecycleState::ENTER_BACKGROUND;
  event_record_.emplace_back(op);
  for (auto& pair : delegates_) {
    if (!(pair.second & op)) {
      pair.first->OnAppEnterBackground();
    }
  }
}

void RuntimeLifecycleObserverImpl::OnRuntimeAttach(Napi::Env current_napi_env) {
  LOGI("[Runtime] RuntimeLifecycleObserverImpl::OnRuntimeAttach:"
       << current_napi_env);
  auto op = LifecycleState::ATTACH;
  event_record_.emplace_back(op);
  args_env_ = current_napi_env;
  for (auto& pair : delegates_) {
    if (!(pair.second & op)) {
      pair.first->OnRuntimeAttach(current_napi_env);
      pair.second |= op;
    }
  }
}

void RuntimeLifecycleObserverImpl::OnRuntimeDetach() {
  LOGI("[Runtime] RuntimeLifecycleObserverImpl::OnRuntimeDetach");
  auto op = LifecycleState::DETACH;
  event_record_.emplace_back(op);
  for (auto& pair : delegates_) {
    if (!(pair.second & op)) {
      pair.first->OnRuntimeDetach();
      pair.second |= op;
    }
  }
}

void RuntimeLifecycleObserverImpl::NotifyListenerChanged() {
  for (auto state : event_record_) {
    for (auto& pair : delegates_) {
      if (!(pair.second & state)) {
        switch (state) {
          case LifecycleState::CREATE:
            pair.first->OnRuntimeCreate(args_vsync_observer_);
            break;
          case LifecycleState::INIT:
            pair.first->OnRuntimeInit(args_runtime_id_);
            break;
          case LifecycleState::ATTACH:
            pair.first->OnRuntimeAttach(static_cast<napi_env>(args_env_));
            break;
          case LifecycleState::DETACH:
            pair.first->OnRuntimeDetach();
            break;
          default:
            break;
        }
        pair.second |= state;
      }
    }
  }
}

void RuntimeLifecycleObserverImpl::AddEventListener(
    std::unique_ptr<RuntimeLifecycleListenerDelegate> listener) {
  auto flag =
      listener->Type() == RuntimeLifecycleListenerDelegate::DelegateType::PART
          ? PART_DELEGATE_FLAG
          : FULL_DELEGATE_FLAG;
  if (flag == FULL_DELEGATE_FLAG) {
    // visible task execute first.
    for (auto state : event_record_) {
      if (state & LifecycleState::ENTER_BACKGROUND) {
        listener->OnAppEnterBackground();
      } else if (state & LifecycleState::ENTER_FOREGROUND) {
        listener->OnAppEnterForeground();
      }
    }
  }
  delegates_.emplace(std::move(listener), flag);
  NotifyListenerChanged();
}

}  // namespace runtime
}  // namespace lynx
