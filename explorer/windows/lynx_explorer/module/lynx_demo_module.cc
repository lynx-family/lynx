// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "explorer/windows/lynx_explorer/module/lynx_demo_module.h"

#include <windows.h>

#include <string>

#include "lynx_window_manager.h"

// LynxTestModule, prefer to use C++ Wrapper.
napi_value openSchema(napi_env env, napi_callback_info info) {
  // unwrap the url param.
  size_t argc = 1;
  napi_value argv[1];
  env->napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  if (argc < 1) {
    return 0;
  }
  char c_url[512] = {0};
  env->napi_get_value_string_utf8(env, argv[0], c_url, sizeof(c_url), nullptr);
  auto* window = lynx::LynxWindowManager::GetInstance().GetFirstWindow();
  if (window) {
    std::string* url_ptr = new std::string(c_url);
    ::PostMessage(window->GetWindowHandle(), LYNX_WINDOW_OPEN, 0,
                  (LPARAM)(url_ptr));
  }
  return 0;
}

napi_value ExplorerModuleCreator(napi_env env, napi_value exports,
                                 const char* module_name, void* opaque) {
  napi_value func;
  env->napi_create_function(env, "openSchema", 1, &openSchema, 0, &func);
  env->napi_set_named_property(env, exports, "openSchema", func);
  return exports;
}
