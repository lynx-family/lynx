// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef EXPLORER_WINDOWS_LYNX_EXPLORER_LYNX_WINDOW_MANAGER_H_
#define EXPLORER_WINDOWS_LYNX_EXPLORER_LYNX_WINDOW_MANAGER_H_

#include <vector>

#include "explorer/windows/lynx_explorer/lynx_window.h"

namespace lynx {

class LynxWindowManager {
 public:
  static LynxWindowManager& GetInstance();

  void PushWindow(LynxWindow* window);
  void RemoveWindow(LynxWindow* window);
  void CloseAllWindow();

  LynxWindow* GetFirstWindow();

  LynxWindowManager(const LynxWindowManager&) = delete;
  LynxWindowManager& operator=(const LynxWindowManager&) = delete;

 private:
  LynxWindowManager() = default;
  ~LynxWindowManager() = default;

  std::vector<LynxWindow*> windows_;
};

}  // namespace lynx

#endif  // EXPLORER_WINDOWS_LYNX_EXPLORER_LYNX_WINDOW_MANAGER_H_
