// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/android/text_utils_android.h"

#include <memory>
#include <string>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/platform/android/jni_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/android/java_only_map.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/TextUtils_jni.h"
#include "core/renderer/dom/android/lepus_message_consumer.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {
namespace {
lepus::Value ConvertJavaData(JNIEnv* env, jobject arguments, jint length) {
  if (arguments == nullptr || length <= 0) {
    return lynx::lepus::Value();
  }

  char* data = static_cast<char*>(env->GetDirectBufferAddress(arguments));
  if (data == nullptr) {
    return lynx::lepus::Value();
  }

  lynx::tasm::LepusDecoder decoder;
  return decoder.DecodeMessage(data, length);
}
}  // namespace

void TextUtilsAndroidHelper::RegisterJNI(JNIEnv* env) {
  RegisterNativesImpl(env);
}

// The input info contains fontSize and fontFamily, both of which are string
// types. The unit of fontSize can be px or rpx.
std::unique_ptr<pub::Value> TextUtilsAndroidHelper::GetTextInfo(
    const std::string& content, const pub::Value& info) {
  JNIEnv* env = base::android::AttachCurrentThread();
  const auto& j_text =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, content);
  const auto& j_font_size =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(
          env, info.GetValueForKey(kFontSize)->str());
  const auto& j_family = base::android::JNIConvertHelper::ConvertToJNIStringUTF(
      env, info.GetValueForKey(kFontFamily)->str());
  const auto max_width_string = info.GetValueForKey(kMaxWidth)->str();

  if (max_width_string.empty()) {
    auto table = lepus::Dictionary::Create();
    auto text_width = Java_TextUtils_getTextWidth(
        env, j_text.Get(), j_font_size.Get(), j_family.Get());
    table->SetValue(BASE_STATIC_STRING(kWidth), text_width);
    return std::make_unique<PubLepusValue>(lepus::Value(table));
  }

  const auto& j_max_width =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env,
                                                             max_width_string);
  const auto max_line_value = info.GetValueForKey(kMaxLine)->Number();

  base::android::ScopedLocalJavaRef<jobject> result =
      Java_TextUtils_getTextInfo(env, j_text.Get(), j_font_size.Get(),
                                 j_family.Get(), j_max_width.Get(),
                                 max_line_value);
  auto java_value = ConvertJavaData(
      env, result.Get(),
      result.Get() ? env->GetDirectBufferCapacity(result.Get()) : 0);
  return std::make_unique<PubLepusValue>(java_value);
}

}  // namespace tasm
}  // namespace lynx
