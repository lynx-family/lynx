// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_ANDROID_JAVA_VALUE_H_
#define CORE_BASE_ANDROID_JAVA_VALUE_H_

#include <climits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/base/js_constants.h"
#include "core/public/jsb/lynx_module_callback.h"
#include "core/public/pub_value.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/js/jsi/jsi.h"
namespace lynx {
namespace base {
namespace android {

// TODO(zhangqun.29) remove convert function in callback_invoker_.cc
namespace converter {
template <typename T>
static jobject valueOf(JNIEnv* env, jclass c, const char* signature,
                       const T& value);
template <typename T>
static double doubleValue(JNIEnv* env, jclass c, const T& value);
jobject charValueOf(JNIEnv* env, jchar value);
jobject byteValueOf(JNIEnv* env, jbyte value);
jobject booleanValueOf(JNIEnv* env, jboolean value);
jobject shortValueOf(JNIEnv* env, jshort value);
jobject integerValueOf(JNIEnv* env, jint value);
jobject longValueOf(JNIEnv* env, jlong value);
jobject floatValueOf(JNIEnv* env, jfloat value);
jobject doubleValueOf(JNIEnv* env, jdouble value);
}  // namespace converter

class JavaValue {
 public:
  enum class JavaValueType {
    Null = 0,
    Undefined,
    Boolean,
    Float,
    Double,
    Int32,
    Int64,
    String,
    ByteArray,
    Array,
    Map,
    // Only use for Java returnType:piperdata
    Transfer,
    // Only use for Java returnType:LynxObject
    LynxObject,

    TemplateData
  };

  JavaValue() : type_(JavaValueType::Null) {}
  JavaValue(bool value);
  JavaValue(double value);
  JavaValue(float value);
  JavaValue(int32_t value);
  JavaValue(int64_t value);
  JavaValue(const std::string& value);
  JavaValue(jstring str);
  JavaValue(const uint8_t* value, size_t length);

  JavaValue(std::shared_ptr<base::android::JavaOnlyArray>&& value)
      : type_(JavaValueType::Array), j_variant_value_(std::move(value)) {}

  JavaValue(std::shared_ptr<base::android::JavaOnlyMap>&& value)
      : type_(JavaValueType::Map), j_variant_value_(std::move(value)) {}
  JavaValue(ScopedGlobalJavaRef<jobject>&& value, JavaValueType type)
      : type_(type), j_variant_value_(std::move(value)) {}

  JavaValue(const JavaValue&) = default;
  JavaValue& operator=(const JavaValue&) = default;
  JavaValue(JavaValue&&) = default;
  JavaValue& operator=(JavaValue&&) = default;

  ~JavaValue() = default;

  bool IsPrimitiveType() {
    return type_ == JavaValueType::Null || type_ == JavaValueType::Boolean ||
           type_ == JavaValueType::Double || type_ == JavaValueType::Int32 ||
           type_ == JavaValueType::Int64 || type_ == JavaValueType::Float ||
           type_ == JavaValueType::Undefined;
  }

  bool IsBool() const { return type_ == JavaValueType::Boolean; }
  bool IsInt32() const { return type_ == JavaValueType::Int32; }
  bool IsInt64() const { return type_ == JavaValueType::Int64; }
  bool IsDouble() const { return type_ == JavaValueType::Double; }
  bool IsFloat() const { return type_ == JavaValueType::Float; }
  bool IsNumber() const {
    return IsInt32() || IsInt64() || IsDouble() || IsFloat();
  }

  bool IsNull() const { return type_ == JavaValueType::Null; }
  bool IsUndefined() const { return type_ == JavaValueType::Undefined; }
  bool IsString() const { return type_ == JavaValueType::String; }
  bool IsArray() const { return type_ == JavaValueType::Array; }
  bool IsArrayBuffer() const { return type_ == JavaValueType::ByteArray; }
  bool IsMap() const { return type_ == JavaValueType::Map; }
  bool IsTransfer() const { return type_ == JavaValueType::Transfer; }
  bool IsLynxObject() const { return type_ == JavaValueType::LynxObject; }
  bool IsTemplateData() const { return type_ == JavaValueType::TemplateData; }

  bool Bool() const;
  int32_t Int32() const;
  int64_t Int64() const;
  double Double() const;
  float Float() const;
  // When using the Number() method on a JavaValue of type Int64, the return
  // value will be cast to double. This will result in a loss of precision when
  // the absolute value exceeds 2^53. If you ensure that the data is within a
  // reasonable range, you can use Number() directly. But it is strongly
  // recommended to use IsInt64() and Int64() when applicable.
  // e.g.
  // if (IsInt64()) {
  //    ...
  // } else if (IsNumber()) {
  //    ...
  // }
  // In this interface , int64 is supported merely to align the behavior in the
  // hierarchy.
  // TODO(zhangqun.29): When Number() in pub::Value is deleted, this interface
  // will be deleted at the same time
  double Number() const;
  uint8_t* ArrayBuffer() const;
  const std::string& String() const;
  const std::shared_ptr<base::android::JavaOnlyArray>& Array() const {
    return std::get<std::shared_ptr<base::android::JavaOnlyArray>>(
        j_variant_value_);
  }
  const std::shared_ptr<base::android::JavaOnlyMap>& Map() const {
    return std::get<std::shared_ptr<base::android::JavaOnlyMap>>(
        j_variant_value_);
  }
  jbyteArray JByteArray() const {
    return static_cast<jbyteArray>(
        std::get<base::android::ScopedGlobalJavaRef<jobject>>(j_variant_value_)
            .Get());
  }
  jstring JString() const {
    return static_cast<jstring>(
        std::get<base::android::ScopedGlobalJavaRef<jobject>>(j_variant_value_)
            .Get());
  }

  jobject TransferData() const {
    return std::get<base::android::ScopedGlobalJavaRef<jobject>>(
               j_variant_value_)
        .Get();
  }

  jobject LynxObject() const {
    return std::get<base::android::ScopedGlobalJavaRef<jobject>>(
               j_variant_value_)
        .Get();
  }

  jobject TemplateData() const {
    return std::get<base::android::ScopedGlobalJavaRef<jobject>>(
               j_variant_value_)
        .Get();
  }

  // Map Getter
  JavaValue GetValueForKey(const std::string& key) const;
  // Array Getter
  JavaValue GetValueForIndex(uint32_t index) const;

  JavaValueType type() const { return type_; }

  // Wrapper JValue
  jvalue JByte() const;
  jvalue WrapperJByte() const;
  jvalue JChar() const;
  jvalue WrapperJChar() const;
  jvalue JBoolean() const;
  jvalue WrapperJBoolean() const;
  jvalue JShort() const;
  jvalue WrapperJShort() const;
  jvalue JInt() const;
  jvalue WrapperJInt() const;
  jvalue JLong() const;
  jvalue WrapperJLong() const;
  jvalue JFloat() const;
  jvalue WrapperJFloat() const;
  jvalue JDouble() const;
  jvalue WrapperJDouble() const;
  jvalue JNull() const;

  int Length() const;

  static JavaValue Undefined() { return JavaValue(JavaValueType::Undefined); }

 private:
  JavaValue(JavaValueType type) : type_(type) {}

  JavaValueType type_{JavaValueType::Null};
  std::variant<jvalue, base::android::ScopedGlobalJavaRef<jobject>,
               std::shared_ptr<base::android::JavaOnlyArray>,
               std::shared_ptr<base::android::JavaOnlyMap>>
      j_variant_value_;
  mutable std::optional<std::string> string_cache_;
  mutable std::vector<uint8_t> array_buffer_ptr_cache_;
};

}  // namespace android
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_ANDROID_JAVA_VALUE_H_
