// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/platform/android/jni_headers/TraceEvent_jni.h"
#include "base/trace/native/platform/android/trace_event_android.h"

void BeginSection(JNIEnv* env, jclass jcaller, jstring category,
                  jstring sectionName) {}
// static
void BeginSectionWithProps(JNIEnv* env, jclass jcaller, jstring category,
                           jstring sectionName, jobject props) {}

// static
void EndSection(JNIEnv* env, jclass jcaller, jstring category,
                jstring sectionName) {}

// static
void EndSectionWithProps(JNIEnv* env, jclass jcaller, jstring category,
                         jstring sectionName, jobject props) {}

// static
static jboolean CategoryEnabled(JNIEnv* env, jclass jcaller, jstring category) {
  return false;
}

// static
void Instant(JNIEnv* env, jclass jcaller, jstring category, jstring sectionName,
             jlong timestamp, jstring color) {}

// static
void InstantWithProps(JNIEnv* env, jclass jcaller, jstring category,
                      jstring sectionName, jlong timestamp, jobject props) {}

void Counter(JNIEnv* env, jclass jcaller, jstring category, jstring eventName,
             jlong counterValue) {}

// static
jboolean SystemTraceEnabled(JNIEnv* env, jclass jcaller) { return false; }
// static
jboolean PerfettoTraceEnabled(JNIEnv* env, jclass jcaller) { return false; }

namespace lynx {
namespace trace {

bool TraceEventAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace trace
}  // namespace lynx
