// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_OWNER_WINDOWS_H_
#define DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_OWNER_WINDOWS_H_

#include <windows.h>

#include <memory>
#include <mutex>
#include <unordered_map>

#include "devtool/base_devtool/logbox/logbox_base.h"

namespace lynx {
namespace devtool {
class LogBoxOwnerWindows : public LogBoxOwner {
 public:
  LogBoxOwnerWindows() = default;
  virtual ~LogBoxOwnerWindows() = default;

  std::shared_ptr<LogBoxManager> CreateManagerForWindow(HWND hWnd);
  std::shared_ptr<LogBoxManager> GetOrCreateLogBoxManager(
      void* native_context) override;
  static void CALLBACK MonitorEventHandle(HWINEVENTHOOK event_hook, DWORD event,
                                          HWND hwnd, LONG object, LONG child,
                                          DWORD event_thread, DWORD event_time);

 private:
  std::mutex mutex_;
  std::unordered_map<HWND, std::shared_ptr<LogBoxManager>> managers_map_;
  std::unordered_map<HWND, HWINEVENTHOOK> hooks_map_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_LOGBOX_LOGBOX_OWNER_WINDOWS_H_
