// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/tasm_platform_invoker_provider_android.h"

#include "base/include/platform/android/jni_utils.h"
#include "core/shell/android/tasm_platform_invoker_android.h"
#include "platform/android/lynx_android/src/main/jni/gen/TasmPlatformInvokerFactory_jni.h"

namespace lynx {
namespace shell {

TasmPlatformInvokerProviderAndroid::TasmPlatformInvokerProviderAndroid(
    JNIEnv* env, jobject native_facade)
    : native_facade_(env, native_facade) {}

std::unique_ptr<TasmPlatformInvoker>
TasmPlatformInvokerProviderAndroid::Create() {
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!env || native_facade_.IsNull()) {
    return nullptr;
  }

  auto invoker =
      Java_TasmPlatformInvokerFactory_createInvoker(env, native_facade_.Get());
  if (!invoker.Get()) {
    return nullptr;
  }

  return std::make_unique<TasmPlatformInvokerAndroid>(env, invoker.Get());
}

}  // namespace shell
}  // namespace lynx
