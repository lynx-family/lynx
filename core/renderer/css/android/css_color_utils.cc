// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/android/css_color_utils.h"

#include <string>

#include "core/base/android/jni_helper.h"
#include "core/build/gen/ColorUtils_jni.h"
#include "core/renderer/css/css_color.h"

jint Parse(JNIEnv* env, jclass jcaller, jstring color) {
  std::string str_color =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, color);
  lynx::tasm::CSSColor hex_color;
  lynx::tasm::CSSColor::Parse(str_color, hex_color);
  return hex_color.Cast();
}

jboolean Validate(JNIEnv* env, jclass jcaller, jstring color) {
  std::string str_color =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, color);
  lynx::tasm::CSSColor hex_color;
  return lynx::tasm::CSSColor::Parse(str_color, hex_color);
}

namespace lynx {
namespace tasm {

bool CSSColorUtils::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace tasm
}  // namespace lynx
