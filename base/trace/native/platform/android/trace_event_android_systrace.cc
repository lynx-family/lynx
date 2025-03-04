// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <dlfcn.h>

#include <string>

#include "base/trace/native/platform/android/jni_headers/TraceEvent_jni.h"
#include "base/trace/native/platform/android/trace_event_android.h"
#include "base/trace/native/trace_event.h"
#include "core/base/android/jni_helper.h"

void BeginSection(JNIEnv* env, jclass jcaller, jstring category,
                  jstring sectionName) {
  const std::string& name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, sectionName);
  TRACE_EVENT_BEGIN(nullptr, name.c_str());
}
// static
void BeginSectionWithProps(JNIEnv* env, jclass jcaller, jstring category,
                           jstring sectionName, jobject props) {
  const std::string& name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, sectionName);
  TRACE_EVENT_BEGIN(nullptr, name.c_str());
}

// static
void EndSection(JNIEnv* env, jclass jcaller, jstring category,
                jstring sectionName) {
  TRACE_EVENT_END(nullptr);
}

// static
void EndSectionWithProps(JNIEnv* env, jclass jcaller, jstring category,
                         jstring sectionName, jobject props) {
  TRACE_EVENT_END(nullptr);
}

// static
jboolean CategoryEnabled(JNIEnv* env, jclass jcaller, jstring category) {
  return TRACE_EVENT_CATEGORY_ENABLED(nullptr);
}

// static
void Instant(JNIEnv* env, jclass jcaller, jstring category, jstring sectionName,
             jlong timestamp, jstring color) {}

// static
void InstantWithProps(JNIEnv* env, jclass jcaller, jstring category,
                      jstring sectionName, jlong timestamp, jobject props) {}

// static
void Counter(JNIEnv* env, jclass jcaller, jstring category, jstring name,
             jlong counterValue) {}

// static
jboolean SystemTraceEnabled(JNIEnv* env, jclass jcaller) { return true; }
// static
jboolean PerfettoTraceEnabled(JNIEnv* env, jclass jcaller) { return false; }

namespace lynx {
namespace trace {

static void RuntimeLoadAtrace() {
  LOGI("RuntimeLoadAtrace");
  void* lib = dlopen("libandroid.so", RTLD_NOW || RTLD_LOCAL);
  if (lib != nullptr) {
    auto atrace_beginsection =
        reinterpret_cast<lynx::trace::ATrace_beginSection_ptr>(
            dlsym(lib, "ATrace_beginSection"));
    auto atrace_endsection =
        reinterpret_cast<lynx::trace::ATrace_endSection_ptr>(
            dlsym(lib, "ATrace_endSection"));
    auto atrace_beginasyncsection =
        reinterpret_cast<lynx::trace::ATrace_beginAsyncSection_ptr>(
            dlsym(lib, "ATrace_beginAsyncSection"));
    auto atrace_endasyncsection =
        reinterpret_cast<lynx::trace::ATrace_endAsyncSection_ptr>(
            dlsym(lib, "ATrace_endAsyncSection"));

    if (atrace_beginsection == nullptr || atrace_endsection == nullptr) {
      LOGE("can not get ATrace_beginSection or ATrace_endSection");
      return;
    }
    lynx::trace::InitSystraceBeginSection(atrace_beginsection);
    lynx::trace::InitSystraceEndSection(atrace_endsection);

    if (atrace_beginasyncsection == nullptr ||
        atrace_endasyncsection == nullptr) {
      LOGE("can not get ATrace_beginAsyncSection or ATrace_endAsyncSection");
    }

    lynx::trace::InitSystraceBeginAsynSection(atrace_beginasyncsection);
    lynx::trace::InitSystraceEndAsynSection(atrace_endasyncsection);
  } else {
    LOGE("ATrace test_lib called failed, ");
  }
}

bool TraceEventAndroid::RegisterJNI(JNIEnv* env) {
  lynx::trace::RuntimeLoadAtrace();
  return RegisterNativesImpl(env);
}

}  // namespace trace
}  // namespace lynx
