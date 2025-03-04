// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_I18N_I18N_BINDER_ANDROID_H_
#define CORE_RENDERER_TASM_I18N_I18N_BINDER_ANDROID_H_

#include <jni.h>

#include "core/renderer/tasm/i18n/i18n.h"

namespace lynx {
namespace tasm {
class I18nBinderAndroid : public I18nBinder {
 public:
  static void RegisterJNI(JNIEnv *env);
  I18nBinderAndroid() = default;
  ~I18nBinderAndroid() = default;

  void Bind(intptr_t ptr);
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_I18N_I18N_BINDER_ANDROID_H_
