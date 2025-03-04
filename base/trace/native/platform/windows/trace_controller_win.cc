// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/platform/windows/trace_controller_win.h"

#include <Shlobj.h>
#include <Shlwapi.h>
#include <Windows.h>
#include <time.h>

#include <memory>
#include <utility>

namespace lynx {
namespace trace {

TraceController* GetTraceControllerInstance() {
  static bool should_init_delegate = true;
  if (should_init_delegate) {
    auto delegate = std::make_unique<TraceControllerDelegateWin>();
    TraceController::Instance()->SetDelegate(std::move(delegate));
    should_init_delegate = false;
  }
  return TraceController::Instance();
}

// static
std::string TraceControllerDelegateWin::GenerateTracingFileDir() {
  // put trace file on the desktop
  char path[MAX_PATH];
  ::SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path);
  return path;
}

}  // namespace trace
}  // namespace lynx
