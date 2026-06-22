// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/android/global_devtool_platform_android.h"

#include <sys/system_properties.h>

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/jni_helper.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/GlobalDevToolPlatformAndroidDelegate_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/GlobalDevToolPlatformAndroidDelegate_register_jni.h"

namespace {

using MemoryUsageCallback =
    lynx::devtool::GlobalDevToolPlatformFacade::MemoryUsageCallback;

constexpr char kEmptyMemoryUsageResultJson[] = "{}";
constexpr char kAndroidMemoryUsageJniException[] =
    "Memory.getAllMemoryUsage failed on Android (JNI exception)";
constexpr char kAndroidMemoryUsageInvalidResult[] =
    "Memory.getAllMemoryUsage returned empty result JSON on Android";

struct AndroidMemoryUsageCallbackState {
  explicit AndroidMemoryUsageCallbackState(MemoryUsageCallback&& callback)
      : callback(std::move(callback)) {}

  std::mutex mutex;
  MemoryUsageCallback callback;
};

using AndroidMemoryUsageCallbackStateHandle =
    std::shared_ptr<AndroidMemoryUsageCallbackState>;

void CompleteMemoryUsageCallback(
    const AndroidMemoryUsageCallbackStateHandle& callback_state,
    const std::string& result_json, const std::string& error_message) {
  if (!callback_state) {
    return;
  }
  MemoryUsageCallback callback;
  {
    std::lock_guard<std::mutex> lock(callback_state->mutex);
    if (!callback_state->callback) {
      return;
    }
    callback = std::move(callback_state->callback);
  }
  std::move(callback)(result_json, error_message);
}

bool IsMemoryUsageCallbackPending(
    const AndroidMemoryUsageCallbackStateHandle& callback_state) {
  if (!callback_state) {
    return false;
  }
  std::lock_guard<std::mutex> lock(callback_state->mutex);
  return static_cast<bool>(callback_state->callback);
}

}  // namespace

static void OnMemoryUsageResult(JNIEnv* env, jclass jcaller, jlong callback_ptr,
                                jstring result_json, jstring error_message) {
  std::unique_ptr<AndroidMemoryUsageCallbackStateHandle> callback_state(
      reinterpret_cast<AndroidMemoryUsageCallbackStateHandle*>(callback_ptr));
  if (!callback_state || !(*callback_state)) {
    return;
  }
  std::string result =
      result_json ? lynx::base::android::JNIConvertHelper::ConvertToString(
                        env, result_json)
                  : kEmptyMemoryUsageResultJson;
  std::string error =
      error_message ? lynx::base::android::JNIConvertHelper::ConvertToString(
                          env, error_message)
                    : "";
  if (result.empty()) {
    result = kEmptyMemoryUsageResultJson;
    if (error.empty()) {
      error = kAndroidMemoryUsageInvalidResult;
    }
  }
  CompleteMemoryUsageCallback(*callback_state, result, error);
}

namespace lynx {
namespace devtool {
namespace jni {

bool RegisterJNIForGlobalDevToolPlatformAndroidDelegate(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace devtool
}  // namespace lynx

namespace lynx {
namespace devtool {

GlobalDevToolPlatformFacade& GlobalDevToolPlatformFacade::GetInstance() {
  static base::NoDestructor<GlobalDevToolPlatformAndroid> instance;
  return *(instance.get());
}

void GlobalDevToolPlatformAndroid::StartMemoryTracing() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_GlobalDevToolPlatformAndroidDelegate_startMemoryTracing(env);
}

void GlobalDevToolPlatformAndroid::StopMemoryTracing() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_GlobalDevToolPlatformAndroidDelegate_stopMemoryTracing(env);
}

void GlobalDevToolPlatformAndroid::GetAllMemoryUsage(
    int64_t timeout_ms, MemoryUsageCallback callback) {
  if (!callback) {
    return;
  }
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto callback_state =
      std::make_shared<AndroidMemoryUsageCallbackState>(std::move(callback));
  auto callback_state_owner =
      std::make_unique<AndroidMemoryUsageCallbackStateHandle>(callback_state);
  jlong callback_ptr = reinterpret_cast<jlong>(callback_state_owner.release());
  Java_GlobalDevToolPlatformAndroidDelegate_queryAllMemoryUsage(env, timeout_ms,
                                                                callback_ptr);
  if (lynx::base::android::HasJNIException()) {
    if (IsMemoryUsageCallbackPending(callback_state)) {
      std::unique_ptr<AndroidMemoryUsageCallbackStateHandle>
          callback_state_cleanup(
              reinterpret_cast<AndroidMemoryUsageCallbackStateHandle*>(
                  callback_ptr));
      CompleteMemoryUsageCallback(callback_state, kEmptyMemoryUsageResultJson,
                                  kAndroidMemoryUsageJniException);
    }
    return;
  }
}

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
lynx::trace::TraceController*
GlobalDevToolPlatformAndroid::GetTraceController() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  intptr_t res =
      Java_GlobalDevToolPlatformAndroidDelegate_getTraceController(env);
  return reinterpret_cast<lynx::trace::TraceController*>(res);
}

lynx::trace::TracePlugin* GlobalDevToolPlatformAndroid::GetFPSTracePlugin() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  intptr_t res =
      Java_GlobalDevToolPlatformAndroidDelegate_getFPSTracePlugin(env);
  return reinterpret_cast<lynx::trace::TracePlugin*>(res);
}

lynx::trace::TracePlugin*
GlobalDevToolPlatformAndroid::GetFrameViewTracePlugin() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  intptr_t res =
      Java_GlobalDevToolPlatformAndroidDelegate_getFrameViewTracePlugin(env);
  return reinterpret_cast<lynx::trace::TracePlugin*>(res);
}

lynx::trace::TracePlugin*
GlobalDevToolPlatformAndroid::GetInstanceTracePlugin() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  intptr_t res =
      Java_GlobalDevToolPlatformAndroidDelegate_getInstanceTracePlugin(env);
  return reinterpret_cast<lynx::trace::TracePlugin*>(res);
}

std::string GlobalDevToolPlatformAndroid::GetLynxVersion() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto lynx_version =
      Java_GlobalDevToolPlatformAndroidDelegate_getLynxVersion(env);
  return lynx::base::android::JNIConvertHelper::ConvertToString(
      env, lynx_version.Get());
}
#endif

std::string GlobalDevToolPlatformAndroid::GetSystemModelName() {
  char value[PROP_VALUE_MAX] = {0};
  int ret = __system_property_get("ro.product.model", value);
  if (ret <= 0) {
    return "";
  }
  return value;
}

}  // namespace devtool
}  // namespace lynx
