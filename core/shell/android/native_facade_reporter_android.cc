// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/native_facade_reporter_android.h"

#include "core/base/android/jni_helper.h"
#include "core/build/gen/NativeFacadeReporter_jni.h"
#include "core/renderer/utils/android/value_converter_android.h"

namespace lynx {
namespace shell {

void NativeFacadeReporterAndroid::RegisterJni(JNIEnv* env) {
  (void)RegisterNativesImpl(env);
}

void NativeFacadeReporterAndroid::OnPerformanceEvent(
    const lepus::Value& entry) {
  // Since this method operates in an asynchronous thread, we need to
  // ensure that the Java Object is available.
  base::android::ScopedLocalJavaRef<jobject> local_ref(jni_object_);
  if (local_ref.IsNull()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_entry_map = lynx::tasm::android::ValueConverterAndroid::
      ConvertLepusToJavaOnlyMapForTiming(entry);
  Java_NativeFacadeReporter_onPerformanceEvent(env, jni_object_.Get(),
                                               j_entry_map.jni_object());
}

}  // namespace shell
}  // namespace lynx
