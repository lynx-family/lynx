// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/piper_data.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/jni_helper.h"
#include "core/base/js_constants.h"
#include "core/renderer/dom/android/lepus_message_consumer.h"
#include "platform/android/lynx_android/src/main/jni/gen/PiperData_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/PiperData_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForPiperData(JNIEnv* env) { return RegisterNativesImpl(env); }
}  // namespace jni
}  // namespace lynx

jlong ParseStringData(JNIEnv* env, jclass jcaller, jstring data) {
  const char* temp = env->GetStringUTFChars(data, JNI_FALSE);

  rapidjson::Document* document = new rapidjson::Document();
  document->Parse(temp);
  env->ReleaseStringUTFChars(data, temp);
  if (document->HasParseError()) {
    LOGE("PiperData Error: source is not valid json format! Error Msg:"
         << document->GetParseErrorMsg()
         << ", position: " << document->GetErrorOffset());
    delete document;
    return 0;
  }
  return reinterpret_cast<jlong>(document);
}

void ReleaseData(JNIEnv* env, jclass jcaller, jlong data) {
  LOGI("ReleaseData:" << data);
  auto json_data = reinterpret_cast<rapidjson::Document*>(data);
  delete json_data;
}

namespace lynx {
namespace base {
namespace android {
namespace {
enum PiperDataType { Empty, String, Map };

std::optional<runtime::js::Value> jsonValueToJSValue(
    const rapidjson::Value& rap_value, runtime::js::Runtime* rt) {
  rapidjson::Type type = rap_value.GetType();
  switch (type) {
    case rapidjson::Type::kNullType:
      return runtime::js::Value(nullptr);
    case rapidjson::Type::kFalseType:
      return runtime::js::Value(false);
    case rapidjson::Type::kTrueType:
      return runtime::js::Value(true);
    case rapidjson::Type::kNumberType: {
      if (rap_value.IsInt()) {
        return runtime::js::Value(rap_value.GetInt());
      }
      if (rap_value.IsInt64()) {
        // In JavaScript,  the max safe integer is 9007199254740991 and the min
        // safe integer is -9007199254740991, so when integer beyond limit, use
        // BigInt Object to define it. More information from
        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number
        int64_t num = rap_value.GetInt64();
        if (num < runtime::js::kMinJavaScriptNumber ||
            num > runtime::js::kMaxJavaScriptNumber) {
          auto bigint_opt =
              runtime::js::BigInt::createWithString(*rt, std::to_string(num));
          if (!bigint_opt) {
            return std::optional<runtime::js::Value>();
          }
          return runtime::js::Value(std::move(*bigint_opt));
        }
        // cast to double
        return runtime::js::Value(rap_value.GetDouble());
      }
      return runtime::js::Value(rap_value.GetDouble());
    }
    case rapidjson::Type::kStringType: {
      return runtime::js::Value(
          runtime::js::String::createFromUtf8(*rt, rap_value.GetString()));
    }
    case rapidjson::Type::kArrayType: {
      auto array_opt = runtime::js::Array::createWithLength(
          *rt, static_cast<size_t>(rap_value.Size()));
      if (!array_opt) {
        return std::optional<runtime::js::Value>();
      }
      for (rapidjson::SizeType i = 0; i < rap_value.Size(); i++) {
        auto js_value_opt = jsonValueToJSValue(rap_value[i], rt);
        if (!js_value_opt) {
          return std::optional<runtime::js::Value>();
        }
        array_opt->setValueAtIndex(*rt, i, std::move(*js_value_opt));
      }
      return runtime::js::Value(std::move(*array_opt));
    }
    case rapidjson::Type::kObjectType: {
      runtime::js::Object obj = runtime::js::Object(*rt);
      for (rapidjson::Value::ConstMemberIterator itr = rap_value.MemberBegin();
           itr != rap_value.MemberEnd(); ++itr) {
        auto js_value_opt = jsonValueToJSValue(itr->value, rt);
        if (!js_value_opt) {
          return std::optional<runtime::js::Value>();
        }
        obj.setProperty(*rt, itr->name.GetString(), std::move(*js_value_opt));
      }
      return runtime::js::Value(obj);
    }
    default:
      break;
  }
  return runtime::js::Value();
}
}  // namespace

std::optional<runtime::js::Value> PiperData::jsObjectFromPiperData(
    JNIEnv* env, runtime::js::Runtime* rt, jobject piper_data) {
  // Notice: You should make sure Java object `PiperData` alive when you
  // call JNI method `getNativePtr`. Otherwise Java object may be dealloc in
  // other thread and the raw native ptr will be free in other thread. Use
  // ScopedLocalJavaRef to ensure `PiperData` alive here.
  std::optional<lynx::runtime::js::Value> ret = runtime::js::Value();
  PiperDataType data_type =
      static_cast<PiperDataType>(Java_PiperData_getDataType(env, piper_data));
  if (data_type == String) {
    jlong json_data = Java_PiperData_getNativePtr(env, piper_data);
    if (!json_data) {
      return runtime::js::Value();
    }
    ret = jsonValueToJSValue(*reinterpret_cast<rapidjson::Document*>(json_data),
                             rt);
  }

  if (data_type == Map) {
    size_t len =
        static_cast<size_t>(Java_PiperData_getBufferPosition(env, piper_data));
    if (len == 0) {
      return runtime::js::Value();
    }
    auto buffer = Java_PiperData_getBuffer(env, piper_data);

    char* message_data =
        static_cast<char*>(env->GetDirectBufferAddress(buffer.Get()));
    lynx::tasm::LepusDecoder decoder;
    ret = decoder.DecodeJSMessage(*rt, message_data, len);
  }

  // If piper data is disposable, recycle it immediately.
  if (data_type != Empty) {
    Java_PiperData_recycleIfIsDisposable(env, piper_data);
  }
  return ret;
}

}  // namespace android
}  // namespace base
}  // namespace lynx
