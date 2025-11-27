// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef EXPLORER_WINDOWS_LYNX_EXPLORER_MODULE_LYNX_DEMO_MODULE_H_
#define EXPLORER_WINDOWS_LYNX_EXPLORER_MODULE_LYNX_DEMO_MODULE_H_

#include "lynx_native_module.h"

napi_value ExplorerModuleCreator(napi_env env, napi_value exports,
                                 const char* module_name, void* opaque);

#endif  // EXPLORER_WINDOWS_LYNX_EXPLORER_MODULE_LYNX_DEMO_MODULE_H_
