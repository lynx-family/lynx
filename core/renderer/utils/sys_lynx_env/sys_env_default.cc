// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace tasm {

long LynxEnv::GetV8Enabled() {
#if OS_ANDROID
  return (IsDevToolEnabled() || IsDevToolEnabledForDebuggableView())
             ? GetLongEnv(Key::ENABLE_V8, 2, EnvType::LOCAL)
             : 0;
#else
  return (IsDevToolEnabled() || IsDevToolEnabledForDebuggableView()) &&
         GetLongEnv(Key::ENABLE_V8, 0, EnvType::LOCAL);
#endif
}

bool LynxEnv::IsQuickjsDebugEnabled() {
  return (IsDevToolEnabled() || IsDevToolEnabledForDebuggableView()) &&
         GetBoolEnv(Key::ENABLE_QUICKJS_DEBUG, true, EnvType::LOCAL);
}

bool LynxEnv::IsJsDebugEnabled(bool force_use_lightweight_js_engine) {
  auto quickjs_enable = IsQuickjsDebugEnabled();
  auto v8_enable = GetV8Enabled();
  if (!quickjs_enable &&
      (!v8_enable || (v8_enable == 2 && force_use_lightweight_js_engine))) {
    return false;
  }
  return true;
}

}  // namespace tasm
}  // namespace lynx
