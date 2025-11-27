// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <windows.h>

#include "explorer/windows/lynx_explorer/lynx_window.h"
#include "explorer/windows/lynx_explorer/lynx_window_manager.h"
#include "explorer/windows/lynx_explorer/module/lynx_demo_module.h"
#include "lynx_env.h"

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev,
                      _In_ wchar_t* command_line, _In_ int show_command) {
  auto& lynx_env = lynx::pub::LynxEnv::GetInstance();
  lynx_env.SetDevtoolEnabled(true);

  lynx_env.RegisterNativeModule("ExplorerModule", ExplorerModuleCreator,
                                nullptr);

  auto* window = new lynx::LynxWindow(0, 0, 800, 600);
  window->SetQuitOnClose(true);
  window->LoadTemplate("");

  ::MSG msg;
  while (::GetMessage(&msg, nullptr, 0, 0)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
  return 0;
}
