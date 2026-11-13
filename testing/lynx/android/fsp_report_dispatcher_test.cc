// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/event_report/android/fsp_report_dispatcher_android.h"
#include "testing/lynx/android/gen/FSPReportDispatcherTestHelper_jni.h"

extern "C" JNIEXPORT jboolean JNICALL
Java_com_lynx_tasm_performance_fsp_FSPReportDispatcherTestHelper_registerJNI(
    JNIEnv* env, jclass /*jclazz*/) {
  return RegisterNativesImpl(env);
}

jint GetActiveGlobalRefCount(JNIEnv* /*env*/, jclass /*jcaller*/) {
  return lynx::tasm::report::GetFSPReportDispatcherGlobalRefCountForTesting();
}
