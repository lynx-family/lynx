// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Copyright 2019 The Lynx Authors. All rights reserved.

#include "core/renderer/dom/android/lynx_view_data_manager_android.h"

#include <memory>
#include <vector>

#include "base/include/log/logging.h"
#include "base/include/platform/android/jni_convert_helper.h"
#include "core/build/gen/TemplateData_jni.h"
#include "core/renderer/data/android/platform_data_android.h"
#include "core/renderer/data/platform_data.h"
#include "core/renderer/data/template_data.h"
#include "core/renderer/dom/android/lepus_message_consumer.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/table.h"

jlong ParseStringData(JNIEnv* env, jclass jcaller, jstring jni_temp) {
  const char* temp = env->GetStringUTFChars(jni_temp, JNI_FALSE);
  lynx::lepus::Value* json = lynx::tasm::LynxViewDataManager::ParseData(temp);
  env->ReleaseStringUTFChars(jni_temp, temp);
  return reinterpret_cast<int64_t>(json);
}

jlong ParseData(JNIEnv* env, jclass jcaller, jobject data, jint length) {
  if (length == 0 || data == nullptr) {
    return 0;
  }
  char* message_data = static_cast<char*>(env->GetDirectBufferAddress(data));
  lynx::lepus::Value* value = new lynx::lepus::Value;
  auto len = static_cast<uint32_t>(length);
  lynx::tasm::LepusDecoder decoder;
  *value = decoder.DecodeMessage(message_data, len);
  return reinterpret_cast<int64_t>(value);
}

void UpdateData(JNIEnv* env, jclass jcaller, jlong nativePtr, jobject data,
                jint length) {
  if (length > 0 && data != nullptr) {
    char* message_data = static_cast<char*>(env->GetDirectBufferAddress(data));
    if (message_data != nullptr) {
      auto len = static_cast<uint32_t>(length);
      lynx::tasm::LepusDecoder decoder;
      lepus_value value = decoder.DecodeMessage(message_data, len);
      auto baseValue = reinterpret_cast<lynx::lepus::Value*>(nativePtr);
      if (value.IsTable()) {
        lynx::lepus::Dictionary* dict = value.Table().get();
        for (const auto& [outer_key, outer_value] : *dict) {
          if (outer_value.IsTable()) {
            lynx::lepus::Value old_value =
                baseValue->Table().get()->GetValue(outer_key);
            if (old_value.IsTable()) {
              if (old_value.Table()->IsConst()) {
                old_value = lynx::lepus::Value::Clone(old_value);
              }
              // merge Table.
              lynx::lepus::Dictionary* table = outer_value.Table().get();
              for (const auto& [key, value] : *table) {
                old_value.Table()->SetValue(key, value);
              }
              baseValue->Table()->SetValue(outer_key, old_value);
              continue;
            }
          }
          baseValue->Table().get()->SetValue(outer_key, outer_value);
        }
      }
    }
  }
}

jobject GetData(JNIEnv* env, jclass jcaller, jlong nativePtr) {
  if (nativePtr == 0) {
    return nullptr;
  }
  auto base_value = reinterpret_cast<lynx::lepus::Value*>(nativePtr);
  lynx::tasm::LepusEncoder encoder;
  std::vector<int8_t> encoded_data = encoder.EncodeMessage(*base_value);
  auto buffer =
      env->NewDirectByteBuffer(encoded_data.data(), encoded_data.size());
  auto result_from_java = Java_TemplateData_decodeByteBuffer(env, buffer);
  return env->NewLocalRef(result_from_java.Get());  // NOLINT
}

void MergeTemplateData(JNIEnv* env, jclass jcaller, jlong destPtr,
                       jlong srcPtr) {
  if (destPtr == 0) {
    LOGW("MergeTemplateData failed since destPtr is nullptr.")
    return;
  }
  if (srcPtr == 0) {
    LOGW("MergeTemplateData failed since srcPtr is nullptr.")
    return;
  }

  auto destValue = reinterpret_cast<lynx::lepus::Value*>(destPtr);
  auto srcValue = reinterpret_cast<lynx::lepus::Value*>(srcPtr);
  if (destValue->IsTable() && destValue->Table()->IsConst()) {
    *destValue = lynx::lepus::Value::Clone(*destValue);
  }
  lynx::tasm::ForEachLepusValue(*srcValue,
                                [&destValue](const lynx::lepus::Value& key,
                                             const lynx::lepus::Value& value) {
                                  destValue->SetProperty(key.String(), value);
                                });
}

void ReleaseData(JNIEnv* env, jclass jcaller, jlong data) {
  auto json_data = reinterpret_cast<lynx::lepus::Value*>(data);
  lynx::tasm::LynxViewDataManager::ReleaseData(json_data);
}

jlong Clone(JNIEnv* env, jclass jcaller, jlong nativePtr) {
  auto base_value = reinterpret_cast<lynx::lepus::Value*>(nativePtr);
  auto new_value = new lynx::lepus::Value;
  if (base_value != nullptr) {
    *new_value = lynx::lepus::Value::Clone(*base_value);
  } else {
    LOGW("LynxViewDataManagerAndroid Clone failed since input is nullptr.")
    *new_value = lynx::lepus::Value::CreateObject();
  }
  return reinterpret_cast<int64_t>(new_value);
}

jlong ShallowCopy(JNIEnv* env, jclass jcaller, jlong nativePtr) {
  auto base_value = reinterpret_cast<lynx::lepus::Value*>(nativePtr);
  auto new_value = new lynx::lepus::Value;
  if (base_value != nullptr) {
    *new_value = lynx::lepus::Value::ShallowCopy(*base_value);
  } else {
    LOGW(
        "LynxViewDataManagerAndroid ShallowCopy failed since input is nullptr.")
    *new_value = lynx::lepus::Value::CreateObject();
  }
  return reinterpret_cast<int64_t>(new_value);
}

jlong CreateObject(JNIEnv* env, jclass jcaller) {
  auto new_value = new lynx::lepus::Value;
  *new_value = lynx::lepus::Value::CreateObject();
  return reinterpret_cast<int64_t>(new_value);
}

jlong CreateTemplateData(JNIEnv* env, jclass jcaller, jlong data,
                         jboolean read_only, jstring name,
                         jobject template_data) {
  auto lepus_data = data != 0 ? *(reinterpret_cast<lynx::lepus::Value*>(data))
                              : lynx::lepus::Value();
  auto processor_name =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, name);

  auto ptr = new std::shared_ptr<lynx::tasm::TemplateData>(
      lepus_data.IsNil() ? nullptr
                         : new lynx::tasm::TemplateData(lepus_data, read_only,
                                                        processor_name));

  if (*ptr != nullptr) {
    (*ptr)->SetPlatformData(std::make_unique<lynx::tasm::PlatformDataAndroid>(
        lynx::base::android::ScopedGlobalJavaRef<jobject>(env, template_data)));
  }
  return reinterpret_cast<jlong>(ptr);
}

void ReleaseTemplateData(JNIEnv* env, jclass jcaller, jlong ptr) {
  if (ptr == 0) {
    return;
  }
  auto data_ptr =
      reinterpret_cast<std::shared_ptr<lynx::tasm::TemplateData>*>(ptr);
  delete data_ptr;
}

namespace lynx {
namespace tasm {

bool LynxViewDataManagerAndroid::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

lepus::Value LynxViewDataManagerAndroid::GetJsThreadDataFromTemplateData(
    JNIEnv* env, jobject jni_object) {
  if (jni_object == nullptr) {
    return lepus::Value();
  }
  auto ptr = Java_TemplateData_getDataForJSThread(env, jni_object);

  auto value = ptr ? *(reinterpret_cast<lepus::Value*>(ptr)) : lepus::Value();
  return value;
}

void LynxViewDataManagerAndroid::ConsumeTemplateDataActions(
    JNIEnv* env, jobject jni_object) {
  if (jni_object == nullptr) {
    return;
  }
  Java_TemplateData_consumeUpdateActions(env, jni_object);
}

LynxViewDataManagerAndroid::LynxViewDataManagerAndroid(JNIEnv* env,
                                                       jobject jni_object)
    : jni_object_(env, jni_object) {}

}  // namespace tasm
}  // namespace lynx
