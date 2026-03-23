// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/devtool_wrapper/android/devtool_pool_android.h"

#include "platform/android/lynx_android/src/main/jni/gen/LynxDevToolPool_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxDevToolPool_register_jni.h"

namespace lynx {
namespace jni {

bool RegisterJNIForLynxDevToolPool(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace lynx

jlong CreateDevToolPool(JNIEnv* env, jobject jcaller) {
  auto* ptr = new lynx::devtool::DevToolPoolAndroid(env, jcaller);
  auto* sp_ptr = new std::shared_ptr<lynx::devtool::DevToolPoolAndroid>(ptr);
  return reinterpret_cast<jlong>(sp_ptr);
}

void DestroyDevToolPool(JNIEnv* env, jobject jcaller, jlong poolPtr) {
  auto* sp_ptr =
      reinterpret_cast<std::shared_ptr<lynx::devtool::DevToolPoolAndroid>*>(
          poolPtr);
  (*sp_ptr)->Destroy();
  delete sp_ptr;
}

namespace lynx {
namespace devtool {

DevToolPoolAndroid::DevToolPoolAndroid(JNIEnv* env, jobject jcaller)
    : weak_android_delegate_(env, jcaller) {}

void DevToolPoolAndroid::CreateDevTool() {
  std::lock_guard<std::mutex> lock(mutex_);
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> ref(weak_android_delegate_);
  if (ref.IsNull()) {
    return;
  }
  Java_LynxDevToolPool_createDevTool(env, ref.Get());
}

void DevToolPoolAndroid::PopDevTool() {
  std::lock_guard<std::mutex> lock(mutex_);
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> ref(weak_android_delegate_);
  if (ref.IsNull()) {
    return;
  }
  Java_LynxDevToolPool_popDevTool(env, ref.Get());
}

std::shared_ptr<lepus::InspectorLepusObserver>
DevToolPoolAndroid::OnMTSRuntimeCreated() {
  std::lock_guard<std::mutex> lock(mutex_);
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> ref(weak_android_delegate_);
  if (ref.IsNull()) {
    return nullptr;
  }
  Java_LynxDevToolPool_onMTSRuntimeCreated(env, ref.Get(),
                                           reinterpret_cast<int64_t>(this));
  return DevToolPool::OnMTSRuntimeCreated();
}

void DevToolPoolAndroid::Destroy() {
  std::lock_guard<std::mutex> lock(mutex_);
  JNIEnv* env = base::android::AttachCurrentThread();
  weak_android_delegate_.Reset(env, nullptr);
}

}  // namespace devtool
}  // namespace lynx
