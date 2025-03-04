// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/platform/android/trace_controller_android.h"

#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>

#include "base/include/log/logging.h"
#include "base/trace/native/platform/android/jni_headers/TraceController_jni.h"
#include "base/trace/native/trace_event.h"
#include "core/base/android/jni_helper.h"

static constexpr int kInvalidTraceSessionId = -1;

// static
jlong CreateTraceController(JNIEnv* env, jobject jcaller) {
  static bool should_init_delegate = true;
  if (should_init_delegate) {
    auto delegate =
        std::make_unique<lynx::trace::TraceControllerDelegateAndroid>(env,
                                                                      jcaller);
    lynx::trace::TraceController::Instance()->SetDelegate(std::move(delegate));
    should_init_delegate = false;
  }
  return reinterpret_cast<jlong>(lynx::trace::TraceController::Instance());
}

// static
jint StartTracing(JNIEnv* env, jobject jcaller, jlong ptr, jint buffer_size,
                  jobjectArray include_categories,
                  jobjectArray exclude_categories, jstring trace_file,
                  jboolean enableSystrace) {
  auto* controller = reinterpret_cast<lynx::trace::TraceController*>(ptr);
  if (controller) {
    auto trace_config = std::make_shared<lynx::trace::TraceConfig>();
    trace_config->file_path =
        lynx::base::android::JNIConvertHelper::ConvertToString(env, trace_file);
    trace_config->included_categories = lynx::base::android::JNIConvertHelper::
        ConvertJavaStringArrayToStringVector(env, include_categories);
    trace_config->excluded_categories = lynx::base::android::JNIConvertHelper::
        ConvertJavaStringArrayToStringVector(env, exclude_categories);
    trace_config->buffer_size = buffer_size;
    trace_config->enable_systrace = enableSystrace == JNI_TRUE;
    int session_id = controller->StartTracing(trace_config);

    return session_id;
  }
  return kInvalidTraceSessionId;
}

// static
void StopTracing(JNIEnv* env, jobject jcaller, jlong ptr, jint session_id) {
  if (ptr != 0) {
    auto* controller = reinterpret_cast<lynx::trace::TraceController*>(ptr);
    controller->StopTracing(session_id);
  }
}

// static
void StartStartupTracingIfNeeded(JNIEnv* env, jobject jcaller, jlong ptr) {
  if (ptr != 0) {
    auto* controller = reinterpret_cast<lynx::trace::TraceController*>(ptr);
    controller->StartStartupTracingIfNeeded();
  }
}

namespace lynx {
namespace trace {

// static
bool TraceControllerDelegateAndroid::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

std::string TraceControllerDelegateAndroid::GenerateTracingFileDir() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  auto ref =
      Java_TraceController_generateTracingFileDir(env, weak_owner_.Get());
  return lynx::base::android::JNIConvertHelper::ConvertToString(env, ref.Get());
}

void TraceControllerDelegateAndroid::RefreshATraceTags() {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  Java_TraceController_refreshATraceTags(env, weak_owner_.Get());
}

}  // namespace trace
}  // namespace lynx
