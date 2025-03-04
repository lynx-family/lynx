// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/piper_data.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/jni_helper.h"
#include "core/base/js_constants.h"
#include "core/build/gen/PiperData_jni.h"
#include "core/renderer/dom/android/lepus_message_consumer.h"

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
  auto json_data = reinterpret_cast<rapidjson::Document*>(data);
  delete json_data;
}

namespace lynx {
namespace base {
namespace android {
namespace {
enum PiperDataType { Empty, String, Map };

std::optional<piper::Value> jsonValueToJSValue(
    const rapidjson::Value& rap_value, piper::Runtime* rt) {
  rapidjson::Type type = rap_value.GetType();
  switch (type) {
    case rapidjson::Type::kNullType:
      return piper::Value(nullptr);
    case rapidjson::Type::kFalseType:
      return piper::Value(false);
    case rapidjson::Type::kTrueType:
      return piper::Value(true);
    case rapidjson::Type::kNumberType: {
      if (rap_value.IsInt()) {
        return piper::Value(rap_value.GetInt());
      }
      if (rap_value.IsInt64()) {
        // In JavaScript,  the max safe integer is 9007199254740991 and the min
        // safe integer is -9007199254740991, so when integer beyond limit, use
        // BigInt Object to define it. More information from
        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number
        int64_t num = rap_value.GetInt64();
        if (num < piper::kMinJavaScriptNumber ||
            num > piper::kMaxJavaScriptNumber) {
          auto bigint_opt =
              piper::BigInt::createWithString(*rt, std::to_string(num));
          if (!bigint_opt) {
            return std::optional<piper::Value>();
          }
          return piper::Value(std::move(*bigint_opt));
        }
        // cast to double
        return piper::Value(rap_value.GetDouble());
      }
      return piper::Value(rap_value.GetDouble());
    }
    case rapidjson::Type::kStringType: {
      return piper::Value(
          piper::String::createFromUtf8(*rt, rap_value.GetString()));
    }
    case rapidjson::Type::kArrayType: {
      auto array_opt = piper::Array::createWithLength(
          *rt, static_cast<size_t>(rap_value.Size()));
      if (!array_opt) {
        return std::optional<piper::Value>();
      }
      for (rapidjson::SizeType i = 0; i < rap_value.Size(); i++) {
        auto js_value_opt = jsonValueToJSValue(rap_value[i], rt);
        if (!js_value_opt) {
          return std::optional<piper::Value>();
        }
        array_opt->setValueAtIndex(*rt, i, std::move(*js_value_opt));
      }
      return piper::Value(std::move(*array_opt));
    }
    case rapidjson::Type::kObjectType: {
      piper::Object obj = piper::Object(*rt);
      for (rapidjson::Value::ConstMemberIterator itr = rap_value.MemberBegin();
           itr != rap_value.MemberEnd(); ++itr) {
        auto js_value_opt = jsonValueToJSValue(itr->value, rt);
        if (!js_value_opt) {
          return std::optional<piper::Value>();
        }
        obj.setProperty(*rt, itr->name.GetString(), std::move(*js_value_opt));
      }
      return piper::Value(obj);
    }
    default:
      break;
  }
  return piper::Value();
}
}  // namespace

bool PiperData::RegisterJni(JNIEnv* env) { return RegisterNativesImpl(env); }

std::optional<piper::Value> PiperData::jsObjectFromPiperData(
    JNIEnv* env, piper::Runtime* rt,
    android::ScopedLocalJavaRef<jobject> piper_data) {
  // Notice: You should make sure Java object `PiperData` alive when you
  // call JNI method `getNativePtr`. Otherwise Java object may be dealloc in
  // other thread and the raw native ptr will be free in other thread. Use
  // ScopedLocalJavaRef to ensure `PiperData` alive here.
  std::optional<lynx::piper::Value> ret = piper::Value();
  PiperDataType data_type = static_cast<PiperDataType>(
      Java_PiperData_getDataType(env, piper_data.Get()));
  if (data_type == String) {
    jlong json_data = Java_PiperData_getNativePtr(env, piper_data.Get());
    if (!json_data) {
      return piper::Value();
    }
    ret = jsonValueToJSValue(*reinterpret_cast<rapidjson::Document*>(json_data),
                             rt);
  }

  if (data_type == Map) {
    size_t len = static_cast<size_t>(
        Java_PiperData_getBufferPosition(env, piper_data.Get()));
    if (len == 0) {
      return piper::Value();
    }
    auto buffer = Java_PiperData_getBuffer(env, piper_data.Get());

    char* message_data =
        static_cast<char*>(env->GetDirectBufferAddress(buffer.Get()));
    lynx::tasm::LepusDecoder decoder;
    ret = decoder.DecodeJSMessage(*rt, message_data, len);
  }

  // If piper data is disposable, recycle it immediately.
  if (data_type != Empty) {
    Java_PiperData_recycleIfIsDisposable(env, piper_data.Get());
  }
  return ret;
}

}  // namespace android
}  // namespace base
}  // namespace lynx
