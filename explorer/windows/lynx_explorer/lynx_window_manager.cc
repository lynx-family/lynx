// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "explorer/windows/lynx_explorer/lynx_window_manager.h"

#include "explorer/windows/lynx_explorer/lynx_window.h"

namespace lynx {

LynxWindowManager& LynxWindowManager::GetInstance() {
  static LynxWindowManager instance;
  return instance;
}

void LynxWindowManager::PushWindow(LynxWindow* window) {
  if (std::find(windows_.begin(), windows_.end(), window) != windows_.end()) {
    return;
  }
  windows_.push_back(window);
}

void LynxWindowManager::RemoveWindow(LynxWindow* window) {
  auto it = std::find(windows_.begin(), windows_.end(), window);
  if (it == windows_.end()) {
    return;
  }
  windows_.erase(it);
}

void LynxWindowManager::CloseAllWindow() {
  std::vector<LynxWindow*> windows;
  windows.swap(windows_);
  for (auto window : windows) {
    window->Destroy();
  }
}

LynxWindow* LynxWindowManager::GetFirstWindow() {
  if (windows_.empty()) {
    return nullptr;
  }
  return windows_.front();
}

}  // namespace lynx
