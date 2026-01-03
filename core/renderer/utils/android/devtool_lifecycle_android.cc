// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/android/devtool_lifecycle_android.h"

#include <jni.h>

#include <memory>

#include "base/include/no_destructor.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/devtool_state.h"
#include "platform/android/lynx_android/src/main/jni/gen/DevToolLifecycle_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/DevToolLifecycle_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForDevToolLifecycle(JNIEnv* env) {
  static base::NoDestructor<tasm::DevToolLifecycleAndroid>
      devtool_lifecycle_android;
  tasm::DevToolLifecycle::GetInstance().SetDelegate(
      devtool_lifecycle_android.get());
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

void SyncStateToNative(JNIEnv* env, jobject jcaller, jint value) {
  lynx::tasm::DevToolState state = static_cast<lynx::tasm::DevToolState>(value);
  lynx::tasm::DevToolLifecycle::GetInstance().SyncStateFromPlatform(state);
}

namespace lynx {
namespace tasm {

void DevToolLifecycleAndroid::SyncStateToPlatform(DevToolState state) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_DevToolLifecycle_syncStateFromNative(env, static_cast<int>(state));
}

}  // namespace tasm
}  // namespace lynx
