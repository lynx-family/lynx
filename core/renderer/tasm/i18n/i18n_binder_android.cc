// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/i18n/i18n_binder_android.h"

#include "core/build/gen/ICURegister_jni.h"

namespace lynx {
namespace tasm {

void I18nBinderAndroid::Bind(intptr_t ptr) {
  // android
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_ICURegister_register(env, ptr);
}
void I18nBinderAndroid::RegisterJNI(JNIEnv* env) { RegisterNativesImpl(env); }
}  // namespace tasm
}  // namespace lynx
