// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/common/android/platform_extra_bundle_android.h"

#include "core/build/gen/PlatformExtraBundleHolder_jni.h"

namespace lynx {
namespace tasm {

bool PlatformBundleHolderAndroid::RegisterJNI(JNIEnv *env) {
  return RegisterNativesImpl(env);
}

PlatformBundleHolderAndroid::PlatformBundleHolderAndroid(JNIEnv *env) {
  impl_.Reset(env, Java_PlatformExtraBundleHolder_generateHolder(env));
}

PlatformBundleHolderAndroid::~PlatformBundleHolderAndroid() {
  impl_.ReleaseGlobalRef(nullptr);
}

base::android::ScopedLocalJavaRef<jobject>
PlatformBundleHolderAndroid::QueryPlatformBundle(int32_t signature) {
  JNIEnv *env = base::android::AttachCurrentThread();
  auto bundle =
      Java_PlatformExtraBundleHolder_getBundle(env, impl_.Get(), signature);
  return bundle;
}

void PlatformBundleHolderAndroid::PutPlatformBundle(
    int32_t signature,
    const base::android::ScopedLocalJavaRef<jobject> &bundle) {
  JNIEnv *env = base::android::AttachCurrentThread();
  Java_PlatformExtraBundleHolder_putBundle(env, impl_.Get(), signature,
                                           bundle.Get());
}

PlatformExtraBundleAndroid::PlatformExtraBundleAndroid(
    int32_t signature, PlatformBundleHolderAndroid *holder,
    const base::android::ScopedLocalJavaRef<jobject> &bundle)
    : PlatformExtraBundle(signature, holder) {
  holder->PutPlatformBundle(Signature(), bundle);
}

base::android::ScopedLocalJavaRef<jobject>
PlatformExtraBundleAndroid::GetPlatformBundle() {
  auto holder = static_cast<PlatformBundleHolderAndroid *>(Holder());
  return holder->QueryPlatformBundle(Signature());
}

}  // namespace tasm
}  // namespace lynx
