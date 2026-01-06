// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <memory>

#include "devtool/lynx_devtool/js_debug/helper/js_debug_helper.h"
#include "devtool/lynx_devtool/js_debug/js/quickjs/proxy/js_debug_proxy_quickjs.h"
#include "devtool/lynx_devtool/js_debug/lepus/proxy/js_debug_proxy_lepus.h"

namespace lynx {
namespace devtool {

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  auto quickjs_proxy = std::make_unique<JSDebugProxyQuickJS>();
  JSDebugHelper::GetInstance()->SetQuickJSProxy(std::move(quickjs_proxy));
  auto lepus_proxy = std::make_unique<JSDebugProxyLepus>();
  JSDebugHelper::GetInstance()->SetLepusProxy(std::move(lepus_proxy));

  return JNI_VERSION_1_6;
}

}  // namespace devtool
}  // namespace lynx
