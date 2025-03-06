// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/lynx_engine_proxy_android.h"

#include <string>
#include <utility>

#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/base/android/jni_helper.h"
#include "core/build/gen/LynxEngineProxy_jni.h"
#include "core/renderer/dom/android/lepus_message_consumer.h"
#include "core/renderer/utils/android/value_converter_android.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/shell/lynx_shell.h"
#include "core/value_wrapper/value_impl_lepus.h"

using lynx::lepus::Value;

Value ConvertJavaData(JNIEnv *env, jobject j_data, jint length) {
  if (j_data == nullptr || length <= 0) {
    return Value();
  }

  char *data = static_cast<char *>(env->GetDirectBufferAddress(j_data));
  if (data == nullptr) {
    return Value();
  }

  lynx::tasm::LepusDecoder decoder;
  return decoder.DecodeMessage(data, length);
}

void InvokeLepusApiCallback(JNIEnv *env, jobject jcaller, jlong nativePtr,
                            jint callbackID, jstring entryName, jobject data) {
  if (!nativePtr) {
    return;
  }
  lynx::shell::LynxEngineProxyAndroid *engine_proxy =
      reinterpret_cast<lynx::shell::LynxEngineProxyAndroid *>(nativePtr);
  engine_proxy->InvokeLepusApiCallback(env, jcaller, nativePtr, callbackID,
                                       entryName, data);
}

static void SendTouchEvent(JNIEnv *env, jobject jcaller, jlong ptr,
                           jstring name, jint tag, jfloat client_x,
                           jfloat client_y, jfloat page_x, jfloat page_y,
                           jfloat view_x, jfloat view_y, jlong timestamp) {
  lynx::shell::LynxEngineProxyAndroid *engine_proxy =
      reinterpret_cast<lynx::shell::LynxEngineProxyAndroid *>(ptr);
  engine_proxy->SendTouchEvent(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, name),
      static_cast<int32_t>(tag), view_x, view_y, client_x, client_y, page_x,
      page_y, static_cast<int64_t>(timestamp));
}

static void SendMultiTouchEvent(JNIEnv *env, jobject jcaller, jlong ptr,
                                jstring name, jobject params, jint length,
                                jlong timestamp) {
  lynx::shell::LynxEngineProxyAndroid *engine_proxy =
      reinterpret_cast<lynx::shell::LynxEngineProxyAndroid *>(ptr);
  engine_proxy->SendTouchEvent(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, name),
      PubLepusValue(ConvertJavaData(env, params, length)),
      static_cast<int64_t>(timestamp));
}

static void SendCustomEvent(JNIEnv *env, jobject jcaller, jlong ptr,
                            jstring name, jint tag, jobject params, jint length,
                            jstring params_name) {
  lynx::shell::LynxEngineProxyAndroid *engine_proxy =
      reinterpret_cast<lynx::shell::LynxEngineProxyAndroid *>(ptr);
  engine_proxy->SendCustomEvent(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, name),
      static_cast<int32_t>(tag),
      PubLepusValue(ConvertJavaData(env, params, length)),
      lynx::base::android::JNIConvertHelper::ConvertToString(env, params_name));
}

static void SendGestureEvent(JNIEnv *env, jobject jcaller, jlong ptr,
                             jstring name, jint tag, jint gesture_id,
                             jobject params, jint length) {
  lynx::shell::LynxEngineProxyAndroid *engine_proxy =
      reinterpret_cast<lynx::shell::LynxEngineProxyAndroid *>(ptr);
  engine_proxy->SendGestureEvent(
      static_cast<int32_t>(tag), static_cast<int32_t>(gesture_id),
      lynx::base::android::JNIConvertHelper::ConvertToString(env, name),
      PubLepusValue(ConvertJavaData(env, params, length)));
}

static void OnPseudoStatusChanged(JNIEnv *env, jobject jcaller, jlong ptr,
                                  jint id, jint pre_status,
                                  jint current_status) {
  lynx::shell::LynxEngineProxyAndroid *engine_proxy =
      reinterpret_cast<lynx::shell::LynxEngineProxyAndroid *>(ptr);
  engine_proxy->OnPseudoStatusChanged(static_cast<int32_t>(id),
                                      static_cast<int32_t>(pre_status),
                                      static_cast<int32_t>(current_status));
}

jlong Create(JNIEnv *env, jobject jcaller, jlong ptr) {
  auto *shell = reinterpret_cast<lynx::shell::LynxShell *>(ptr);
  lynx::shell::LynxEngineProxyAndroid *engine_proxy =
      new lynx::shell::LynxEngineProxyAndroid(shell->GetEngineActor(), env,
                                              jcaller);
  return reinterpret_cast<jlong>(engine_proxy);
}

void Destroy(JNIEnv *env, jobject jcaller, jlong ptr) {
  delete reinterpret_cast<lynx::shell::LynxEngineProxyAndroid *>(ptr);
}

void DispatchTaskToLynxEngine(JNIEnv *env, jobject jcaller, jlong ptr,
                              jobject java_runnable) {
  auto *engine_proxy =
      reinterpret_cast<lynx::shell::LynxEngineProxyAndroid *>(ptr);

  lynx::base::android::ScopedGlobalJavaRef<jobject> runnable(env,
                                                             java_runnable);

  engine_proxy->DispatchTaskToLynxEngine([runnable = std::move(runnable)]() {
    JNIEnv *env = lynx::base::android::AttachCurrentThread();
    Java_LynxEngineProxy_executeRunnable(env, runnable.Get());
  });
}

namespace lynx {
namespace shell {
bool LynxEngineProxyAndroid::RegisterJNI(JNIEnv *env) {
  return RegisterNativesImpl(env);
}

void LynxEngineProxyAndroid::InvokeLepusApiCallback(
    JNIEnv *env, jobject jcaller, jlong nativePtr, jint callbackID,
    jstring entryName, jobject data) {
  if (engine_actor_ == nullptr) {
    return;
  }
  const std::string entry =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, entryName);
  long length = env->GetDirectBufferCapacity(data);
  Value value;
  if (length > 0) {
    value = ConvertJavaData(env, data, length);
  } else {
    value = Value();
  }
  engine_actor_->Act([callbackID, entry, value](auto &engine) {
    return engine->InvokeLepusCallback(callbackID, entry, value);
  });
}
}  // namespace shell
}  // namespace lynx
