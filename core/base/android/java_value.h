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

#include "core/base/android/android_jni.h"
#include "core/base/android/java_only_array.h"
#include "core/base/android/java_only_map.h"
#include "core/base/js_constants.h"
#include "core/public/jsb/lynx_module_callback.h"
#include "core/public/pub_value.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
namespace lynx {
namespace base {
namespace android {

class JavaValue {
 public:
  enum class JavaValueType {
    Null = 0,
    Boolean,
    Double,
    Int32,
    Int64,
    String,
    ByteArray,
    Array,
    Map,
  };

  JavaValue() : type_(JavaValueType::Null) {}
  JavaValue(bool value);
  JavaValue(double value);
  JavaValue(int32_t value);
  JavaValue(int64_t value);
  JavaValue(const std::string& value);
  JavaValue(jstring str);
  JavaValue(const uint8_t* value, size_t length);

  JavaValue(std::shared_ptr<base::android::JavaOnlyArray>&& value)
      : type_(JavaValueType::Array), j_variant_value_(std::move(value)) {}

  JavaValue(std::shared_ptr<base::android::JavaOnlyMap>&& value)
      : type_(JavaValueType::Map), j_variant_value_(std::move(value)) {}

  JavaValue(const JavaValue&) = default;
  JavaValue& operator=(const JavaValue&) = default;
  JavaValue(JavaValue&&) = default;
  JavaValue& operator=(JavaValue&&) = default;

  ~JavaValue() = default;

  bool IsPrimitiveType() {
    return type_ == JavaValueType::Null || type_ == JavaValueType::Boolean ||
           type_ == JavaValueType::Double || type_ == JavaValueType::Int32 ||
           type_ == JavaValueType::Int64;
  }

  bool IsBool() const { return type_ == JavaValueType::Boolean; }
  bool IsInt32() const { return type_ == JavaValueType::Int32; }
  bool IsInt64() const { return type_ == JavaValueType::Int64; }
  bool IsDouble() const { return type_ == JavaValueType::Double; }
  bool IsNumber() const { return IsInt32() || IsInt64() || IsDouble(); }

  bool IsNull() const { return type_ == JavaValueType::Null; }
  bool IsString() const { return type_ == JavaValueType::String; }
  bool IsArray() const { return type_ == JavaValueType::Array; }
  bool IsArrayBuffer() const { return type_ == JavaValueType::ByteArray; }
  bool IsMap() const { return type_ == JavaValueType::Map; }

  bool Bool() const;
  int32_t Int32() const;
  int64_t Int64() const;
  double Double() const;
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

  // Map Getter
  JavaValue GetValueForKey(const std::string& key) const;
  // Array Getter
  JavaValue GetValueForIndex(uint32_t index) const;

  JavaValueType type() const { return type_; }

 private:
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
