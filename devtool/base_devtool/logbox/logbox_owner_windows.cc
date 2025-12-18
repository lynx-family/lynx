// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/logbox/logbox_owner_windows.h"

#include <tchar.h>
#include <windows.h>
#include <winuser.h>

#include <memory>

#include "devtool/base_devtool/logbox/logbox_manager.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"

namespace lynx {
namespace devtool {

std::shared_ptr<LogBoxOwner> LogBoxOwner::GetInstance() {
  static std::shared_ptr<LogBoxOwner> sp_instance =
      std::make_shared<LogBoxOwnerWindows>();
  return sp_instance;
}

std::shared_ptr<LogBoxManager> LogBoxOwnerWindows::GetOrCreateLogBoxManager(
    void* native_context) {
  if (native_context == nullptr) {
    return nullptr;
  }
  HWND hWnd = static_cast<HWND>(native_context);
  std::lock_guard<std::mutex> guard(mutex_);
  if (managers_map_.find(hWnd) != managers_map_.end()) {
    return managers_map_[hWnd];
  }
  return LogBoxOwnerWindows::CreateManagerForWindow(hWnd);
}

std::shared_ptr<LogBoxManager> LogBoxOwnerWindows::CreateManagerForWindow(
    HWND hWnd) {
  auto manager = std::make_shared<LogBoxManager>((void*)hWnd);

  DWORD proc_id = GetCurrentProcessId();
  auto hook = SetWinEventHook(
      EVENT_OBJECT_DESTROY,         // window destroy indicate to remove manager
      EVENT_OBJECT_LOCATIONCHANGE,  // listen for location and size changes
      nullptr,
      LogBoxOwnerWindows::MonitorEventHandle,  // callback function
      proc_id,                                 // current process id
      0,                                       // all threads
      WINEVENT_OUTOFCONTEXT);
  if (!hook) {
    return nullptr;
  }
  managers_map_[hWnd] = manager;
  hooks_map_[hWnd] = hook;
  return manager;
}

void CALLBACK LogBoxOwnerWindows::MonitorEventHandle(HWINEVENTHOOK event_hook,
                                                     DWORD event, HWND hwnd,
                                                     LONG object, LONG child,
                                                     DWORD event_thread,
                                                     DWORD event_time) {
  if (event != EVENT_OBJECT_DESTROY && event != EVENT_OBJECT_LOCATIONCHANGE)
    return;
  if (object != OBJID_WINDOW || child != CHILDID_SELF) return;

  auto owner =
      std::static_pointer_cast<LogBoxOwnerWindows>(LogBoxOwner::GetInstance());
  CHECK_NULL_RETURN(owner);
  std::lock_guard<std::mutex> guard(owner->mutex_);
  if (owner->managers_map_.find(hwnd) == owner->managers_map_.end()) {
    return;
  }

  auto manager = owner->managers_map_[hwnd];
  if (!manager) {
    return;
  }
  auto hook = owner->hooks_map_[hwnd];
  if (event == EVENT_OBJECT_DESTROY) {
    UnhookWinEvent(hook);
    owner->managers_map_.erase(hwnd);
    owner->hooks_map_.erase(hwnd);
  } else {
    manager->OnNativeWindowChange();
  }
}
}  // namespace devtool
}  // namespace lynx
