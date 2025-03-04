// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <dlfcn.h>

#include <string>

#include "base/trace/native/platform/android/jni_headers/TraceEvent_jni.h"
#include "base/trace/native/platform/android/trace_event_android.h"
#include "base/trace/native/trace_event.h"
#include "base/trace/native/track_event_wrapper.h"
#include "core/base/android/jni_helper.h"

static void UpdateTraceDebugInfo(const jobject& props,
                                 const lynx::perfetto::EventContext& ctx) {
  JNIEnv* env = lynx::base::android::AttachCurrentThread();

  // Gets the entrySet method in the java Map.
  jmethodID entrySetMethod = env->GetMethodID(env->GetObjectClass(props),
                                              "entrySet", "()Ljava/util/Set;");

  // Invoke the entrySet method.
  jobject set = env->CallObjectMethod(props, entrySetMethod);

  // Gets the iterator method in the java Set.
  jmethodID iteratorMethod = env->GetMethodID(
      env->GetObjectClass(set), "iterator", "()Ljava/util/Iterator;");

  // Invoke the iterator method.
  jobject iterator = env->CallObjectMethod(set, iteratorMethod);

  // Gets the Class of the java Iterator.
  jclass iteratorClass = env->GetObjectClass(iterator);

  // Gets hasNext and next methods from java Iterator.
  jmethodID hasNextMethod = env->GetMethodID(iteratorClass, "hasNext", "()Z");
  jmethodID nextMethod =
      env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

  while (env->CallBooleanMethod(iterator, hasNextMethod)) {
    jobject entry = env->CallObjectMethod(iterator, nextMethod);
    jclass entryClass = env->GetObjectClass(entry);

    jmethodID getKeyMethod =
        env->GetMethodID(entryClass, "getKey", "()Ljava/lang/Object;");
    jmethodID getValueMethod =
        env->GetMethodID(entryClass, "getValue", "()Ljava/lang/Object;");

    jstring key = (jstring)env->CallObjectMethod(entry, getKeyMethod);
    jstring value = (jstring)env->CallObjectMethod(entry, getValueMethod);
    const std::string& keyCStr =
        lynx::base::android::JNIConvertHelper::ConvertToString(env, key);
    const std::string& valueCStr =
        lynx::base::android::JNIConvertHelper::ConvertToString(env, value);

    auto* debug = ctx.event()->add_debug_annotations();
    debug->set_name(keyCStr);
    debug->set_string_value(valueCStr);

    env->DeleteLocalRef(entry);
    env->DeleteLocalRef(entryClass);
  }
}

void BeginSection(JNIEnv* env, jclass jcaller, jstring category,
                  jstring sectionName) {
  const std::string& category_name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, category);
  const std::string& name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, sectionName);
  TRACE_EVENT_BEGIN(category_name.c_str(), nullptr,
                    [&name](lynx::perfetto::EventContext ctx) {
                      ctx.event()->set_name(name);
                    });
}
// static
void BeginSectionWithProps(JNIEnv* env, jclass jcaller, jstring category,
                           jstring sectionName, jobject props) {
  const std::string& category_name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, category);
  const std::string& name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, sectionName);
  TRACE_EVENT_BEGIN(category_name.c_str(), nullptr,
                    [&name, &props](lynx::perfetto::EventContext ctx) {
                      ctx.event()->set_name(name);
                      UpdateTraceDebugInfo(props, ctx);
                    });
}

// static
void EndSection(JNIEnv* env, jclass jcaller, jstring category,
                jstring sectionName) {
  const std::string& category_name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, category);
  const std::string& name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, sectionName);
  TRACE_EVENT_END(category_name.c_str(),
                  [&name](lynx::perfetto::EventContext ctx) {
                    ctx.event()->set_name(name);
                  });
}

// static
void EndSectionWithProps(JNIEnv* env, jclass jcaller, jstring category,
                         jstring sectionName, jobject props) {
  const std::string& category_name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, category);
  const std::string& name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, sectionName);
  TRACE_EVENT_END(category_name.c_str(),
                  [&name, &props](lynx::perfetto::EventContext ctx) {
                    ctx.event()->set_name(name);
                    UpdateTraceDebugInfo(props, ctx);
                  });
}

// static
jboolean CategoryEnabled(JNIEnv* env, jclass jcaller, jstring category) {
  const std::string& category_name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, category);
  return TRACE_EVENT_CATEGORY_ENABLED(category_name.c_str());
}

// static
void Instant(JNIEnv* env, jclass jcaller, jstring category, jstring sectionName,
             jlong timestamp, jstring color) {
  const std::string& category_name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, category);
  const std::string& name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, sectionName);
  const std::string& color_string =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, color);
  TRACE_EVENT_INSTANT(
      category_name.c_str(), nullptr,
      [&name, &color_string, &timestamp](lynx::perfetto::EventContext ctx) {
        ctx.event()->set_name(name);
        ctx.event()->set_timestamp_absolute_us(
            static_cast<uint64_t>(timestamp));
        auto* debug = ctx.event()->add_debug_annotations();
        debug->set_name("color");
        debug->set_string_value(color_string);
      });
}

// static
void InstantWithProps(JNIEnv* env, jclass jcaller, jstring category,
                      jstring sectionName, jlong timestamp, jobject props) {
  const std::string& category_name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, category);
  const std::string& name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, sectionName);
  TRACE_EVENT_INSTANT(
      category_name.c_str(), nullptr,
      [&name, &timestamp, &props](lynx::perfetto::EventContext ctx) {
        ctx.event()->set_name(name);
        ctx.event()->set_timestamp_absolute_us(
            static_cast<uint64_t>(timestamp));
        UpdateTraceDebugInfo(props, ctx);
      });
}

// static
void Counter(JNIEnv* env, jclass jcaller, jstring category, jstring name,
             jlong counterValue) {
  const std::string& category_name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, category);
  const std::string& track_name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, name);
  TRACE_COUNTER(category_name.c_str(), track_name.c_str(),
                static_cast<uint64_t>(counterValue));
}

// static
jboolean SystemTraceEnabled(JNIEnv* env, jclass jcaller) { return false; }
// static
jboolean PerfettoTraceEnabled(JNIEnv* env, jclass jcaller) { return true; }

namespace lynx {
namespace trace {

bool TraceEventAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace trace
}  // namespace lynx
