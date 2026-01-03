// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/devtool_lifecycle.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace tasm {

DevToolLifecycle& DevToolLifecycle::GetInstance() {
  static base::NoDestructor<DevToolLifecycle> instance;
  return *instance;
}

void DevToolLifecycle::SetDelegate(Delegate* delegate) {
  delegate_.store(delegate);
}

DevToolState DevToolLifecycle::GetState() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return state_;
}

bool DevToolLifecycle::IsAttached() const {
  return GetState() >= DevToolState::ATTACHED;
}

bool DevToolLifecycle::IsInitialized() const {
  return GetState() >= DevToolState::INITIALIZED;
}

bool DevToolLifecycle::IsEnabled() const {
  return GetState() >= DevToolState::ENABLED;
}

bool DevToolLifecycle::IsConnected() const {
  return GetState() >= DevToolState::CONNECTED;
}

void DevToolLifecycle::OnAttached() {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == DevToolState::UNAVAILABLE) {
      state_ = DevToolState::ATTACHED;
      changed = true;
      LOGI("DevTool attached. Transitioning to ATTACHED.");
    } else {
      LOGW("OnAttached() called but state is " << static_cast<int>(state_));
    }
  }
  if (changed) {
    if (auto delegate = delegate_.load()) {
      delegate->SyncStateToPlatform(DevToolState::ATTACHED);
    }
  }
}

void DevToolLifecycle::OnEnabled() {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == DevToolState::ATTACHED) {
      state_ = DevToolState::ENABLED;
      changed = true;
      LOGI("DevTool enabled. Transitioning to ENABLED.");
    } else if (state_ == DevToolState::UNAVAILABLE) {
      LOGW("Cannot enable DevTool because it is UNAVAILABLE");
    }
  }
  if (changed) {
    if (auto delegate = delegate_.load()) {
      delegate->SyncStateToPlatform(DevToolState::ENABLED);
    }
  }
}

void DevToolLifecycle::OnDisabled() {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == DevToolState::ENABLED ||
        state_ == DevToolState::INITIALIZED ||
        state_ == DevToolState::CONNECTED) {
      state_ = DevToolState::ATTACHED;
      changed = true;
      LOGI("DevTool disabled. Transitioning to ATTACHED.");
    }
  }
  if (changed) {
    if (auto delegate = delegate_.load()) {
      delegate->SyncStateToPlatform(DevToolState::ATTACHED);
    }
  }
}

void DevToolLifecycle::OnInitialized() {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == DevToolState::ENABLED) {
      state_ = DevToolState::INITIALIZED;
      changed = true;
      LOGI("DevTool env initialized. Transitioning to INITIALIZED.");
    }
  }
  if (changed) {
    if (auto delegate = delegate_.load()) {
      delegate->SyncStateToPlatform(DevToolState::INITIALIZED);
    }
  }
}

void DevToolLifecycle::OnConnected() {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == DevToolState::INITIALIZED) {
      state_ = DevToolState::CONNECTED;
      changed = true;
      LOGI("DevTool client connected. Transitioning to CONNECTED.");
    }
  }
  if (changed) {
    if (auto delegate = delegate_.load()) {
      delegate->SyncStateToPlatform(DevToolState::CONNECTED);
    }
  }
}

void DevToolLifecycle::OnDisconnected() {
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == DevToolState::CONNECTED) {
      state_ = DevToolState::INITIALIZED;
      changed = true;
      LOGI("DevTool client disconnected. Transitioning to INITIALIZED.");
    }
  }
  if (changed) {
    if (auto delegate = delegate_.load()) {
      delegate->SyncStateToPlatform(DevToolState::INITIALIZED);
    }
  }
}

void DevToolLifecycle::SyncStateFromPlatform(DevToolState state) {
  std::lock_guard<std::mutex> lock(mutex_);
  // We use direct assignment here instead of calling OnAttached/OnEnabled/etc.
  // because:
  // 1. Ambiguity: Some states (like ATTACHED) can be reached via multiple
  // transitions (OnAttached vs OnDisabled), making it hard to choose the right
  // method.
  // 2. Robustness: Platform sync should be authoritative. If we missed an
  // intermediate event (e.g. ATTACHED), calling OnEnabled() would fail due to
  // strict state checks. We want to force the state to match the platform.
  if (state_ != state) {
    LOGI("SyncStateFromPlatform: Transitioning from "
         << static_cast<int>(state_) << " to " << static_cast<int>(state));
    state_ = state;
  }
}

void DevToolLifecycle::ResetForTesting() {
  std::lock_guard<std::mutex> lock(mutex_);
  state_ = DevToolState::UNAVAILABLE;
}

}  // namespace tasm
}  // namespace lynx
