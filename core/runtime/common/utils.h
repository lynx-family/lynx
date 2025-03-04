// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_COMMON_UTILS_H_
#define CORE_RUNTIME_COMMON_UTILS_H_

#include <string>
#include <utility>
#include <vector>

#if defined(OS_ANDROID)
#include "core/base/android/java_only_array.h"
#endif
#include "base/include/debug/lynx_error.h"
#include "base/include/log/logging.h"
#include "base/include/vector.h"
#include "core/base/json/json_util.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace piper {
class JSIObjectWrapperManager;
std::optional<Value> valueFromLepus(
    Runtime& runtime, const lepus::Value& data,
    JSIObjectWrapperManager* jsi_object_wrapper_manager = nullptr);

std::optional<Array> arrayFromLepus(Runtime& runtime,
                                    const lepus::CArray& array);

// Track the depth of JSValue referencing chain.
using JSValueCircularArray = base::InlineVector<piper::Object, 32>;

std::optional<lepus_value> ParseJSValue(
    piper::Runtime& runtime, const piper::Value& value,
    JSIObjectWrapperManager* jsi_object_wrapper_manager,
    const std::string& jsi_object_group_id, const std::string& targetSDKVersion,
    JSValueCircularArray& pre_object_vector, int depth = 0);

bool IsCircularJSObject(Runtime& runtime, const Object& object,
                        const JSValueCircularArray& pre_object_vector);

bool CheckIsCircularJSObjectIfNecessaryAndReportError(
    Runtime& runtime, const Object& object,
    const JSValueCircularArray& pre_object_vector, int depth,
    const char* message);

// Convert string[] to std::vector<std::string>.
// The input value must be an array and each element in input must be string.
// Otherwise, the conversion will be aborted and return false.
bool ConvertPiperValueToStringVector(Runtime& rt, const piper::Value& input,
                                     std::vector<std::string>& result);

class ScopedJSObjectPushPopHelper {
 public:
  ScopedJSObjectPushPopHelper(JSValueCircularArray& vector,
                              piper::Object object)
      : pre_object_vector_(vector) {
    pre_object_vector_.push_back(std::move(object));
  };
  ~ScopedJSObjectPushPopHelper() { pre_object_vector_.pop_back(); }

 private:
  JSValueCircularArray& pre_object_vector_;
};

#if defined(OS_ANDROID)
bool JSBUtilsRegisterJNI(JNIEnv* env);
bool JSBUtilsMapRegisterJNI(JNIEnv* env);

void PushByteArrayToJavaArray(piper::Runtime* rt,
                              const piper::ArrayBuffer& array_buffer,
                              base::android::JavaOnlyArray* jarray);
void PushByteArrayToJavaMap(piper::Runtime* rt, const std::string& key,
                            const piper::ArrayBuffer& array_buffer,
                            base::android::JavaOnlyMap* jmap);

std::optional<piper::Array> jsArrayFromJavaOnlyArray(JNIEnv* env, jobject array,
                                                     piper::Runtime* rt);
std::optional<piper::Object> jsObjectFromJavaOnlyMap(JNIEnv* env, jobject map,
                                                     piper::Runtime* rt);
#endif  // OS_ANDROID

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_COMMON_UTILS_H_
