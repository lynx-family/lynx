// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/android/src/main/jni/gen/TraceEvent_jni.h"
#include "base/trace/android/src/main/jni/gen/TraceEvent_register_jni.h"

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
namespace jni {
bool RegisterJNIForTraceEvent(JNIEnv* env) { return RegisterNativesImpl(env); }
}  // namespace jni
}  // namespace lynx
