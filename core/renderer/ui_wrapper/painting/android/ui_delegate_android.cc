// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/ui_delegate_android.h"

#include <memory>

#include "core/build/gen/LynxUIRenderer_jni.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"

jlong CreateUIDelegate(JNIEnv* env, jclass jcaller, jlong painting_context_ptr,
                       jlong layout_context_ptr) {
  return reinterpret_cast<jlong>(new lynx::tasm::UIDelegateAndroid(
      painting_context_ptr, layout_context_ptr));
}

void DestroyUIDelegate(JNIEnv* env, jclass jcaller, jlong ui_delegate_ptr) {
  delete reinterpret_cast<lynx::tasm::UIDelegateAndroid*>(ui_delegate_ptr);
}

namespace lynx {
namespace tasm {

bool UIDelegateAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

std::unique_ptr<PaintingCtxPlatformImpl>
UIDelegateAndroid::CreatePaintingContext() {
  return std::unique_ptr<PaintingCtxPlatformImpl>(painting_context_);
}

std::unique_ptr<LayoutCtxPlatformImpl>
UIDelegateAndroid::CreateLayoutContext() {
  return std::unique_ptr<LayoutCtxPlatformImpl>(layout_context_);
}

std::unique_ptr<PropBundleCreator>
UIDelegateAndroid::CreatePropBundleCreator() {
  return std::make_unique<PropBundleCreatorAndroid>();
}

}  // namespace tasm
}  // namespace lynx
