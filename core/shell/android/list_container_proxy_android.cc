// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <jni.h>

#include "core/base/thread/atomic_lifecycle.h"
#include "core/public//list_container_proxy.h"
#include "core/shell/lynx_shell.h"
#include "platform/android/lynx_android/src/main/jni/gen/ListContainerProxy_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/ListContainerProxy_register_jni.h"
namespace lynx {
namespace jni {

bool RegisterJNIForListContainerProxy(JNIEnv *env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

// ———————————— JNI method start ————————————————

jlong Create(JNIEnv *env, jobject jcaller, jlong ptr) {
  auto engine_proxy = reinterpret_cast<lynx::shell::ListEngineProxy *>(ptr);
  auto container_proxy = new lynx::shell::ListContainerProxy(engine_proxy);
  return reinterpret_cast<jlong>(container_proxy);
}

void ScrollByListContainer(JNIEnv *env, jobject jcaller, jlong ptr, jint sign,
                           jfloat dx, jfloat dy, jfloat originalX,
                           jfloat originalY) {
  auto container_proxy =
      reinterpret_cast<lynx::shell::ListContainerProxy *>(ptr);
  container_proxy->ScrollByListContainer(sign, dx, dy, originalX, originalY);
}

void ScrollToPosition(JNIEnv *env, jobject jcaller, jlong ptr, jint sign,
                      jint position, jfloat offset, jint align,
                      jboolean smooth) {
  auto container_proxy =
      reinterpret_cast<lynx::shell::ListContainerProxy *>(ptr);
  container_proxy->ScrollToPosition(sign, position, offset, align, smooth);
}

void ScrollStopped(JNIEnv *env, jobject jcaller, jlong ptr, jint sign) {
  auto container_proxy =
      reinterpret_cast<lynx::shell::ListContainerProxy *>(ptr);
  container_proxy->ScrollStopped(sign);
}

void Destroy(JNIEnv *env, jobject jcaller, jlong ptr) {
  lynx::shell::ListContainerProxy *obj =
      reinterpret_cast<lynx::shell::ListContainerProxy *>(ptr);
  delete obj;
};

// --------   JNI method End ————————————
