// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <memory>

#include "devtool/lynx_devtool/js_debug/helper/js_debug_helper.h"
#include "devtool/lynx_devtool/js_debug/helper/js_debug_proxy_impl.h"

namespace lynx {
namespace devtool {

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  auto proxy = std::make_unique<JSDebugProxyImpl>();
  JSDebugHelper::GetInstance()->SetJSDebugProxy(std::move(proxy));

  return JNI_VERSION_1_6;
}

}  // namespace devtool
}  // namespace lynx
