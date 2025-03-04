// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_COMMON_VALUE_H_
#define BINDING_COMMON_VALUE_H_

#include <string>
#include <variant>
#include <vector>

#include "third_party/binding/common/base.h"
#include "third_party/binding/common/object.h"

namespace Napi {
class Env;
class Value;
}  // namespace Napi

namespace lynx {
namespace binding {

enum class ValueType {
  kEmpty,
  kNull,
  kUndefined,
  kBoolean,
  kNumber,
  kString,
  kArray,
  kTypedArray,
  kArrayBufferView,
  kArrayBuffer,
  kObject,
  kDictionary,
};

// Valid when value type is kArray or kTypedArray.
enum class ArrayType {
  kTypeEmpty,
  kTypeInt8,
  kTypeUint8,
  kTypeUint8Clamped,
  kTypeInt16,
  kTypeUint16,
  kTypeInt32,
  kTypeUint32,
  kTypeFloat32,
  kTypeFloat64,
  kTypeBigInt64,
  kTypeBigUint64,
  kTypeDataView,
  // Above this line all types should correspond to ArrayBufferView::ViewType.
  kTypeBoolean,
  kTypeString,
  kTypeObject,
  kTypeValue,
};

constexpr int kDictionaryInvalidType = -1;

class Value {
 public:
  static Value Null();
  static Value Undefined();
  static Value Boolean(bool b);
  static Value Number(double num);
  static Value String(std::string str);
  static Value Array(std::vector<int32_t> array, ArrayType type);
  static Value Array(std::vector<uint32_t> array, ArrayType type);
  static Value Array(std::vector<float> array, ArrayType type);
  static Value Array(std::vector<double> array, ArrayType type);
  static Value Array(std::vector<std::string> array);
  static Value Array(std::vector<Object> array);
  static Value Array(std::vector<Value> array);
  static Value Int32Array(std::vector<int32_t> array);
  static Value Uint32Array(std::vector<uint32_t> array);
  static Value Float32Array(std::vector<float> array);
  static Value ArrayBufferView(std::vector<char> data, ArrayType type);
  typedef void (*Finalizer)(void*);
  static Value ArrayBuffer(size_t size, void* data = nullptr,
                           Finalizer finalizer = nullptr);
  static Value Object(Object&& obj);
  static Value Dictionary(std::vector<std::pair<std::string, Value>> dict,
                          int type = kDictionaryInvalidType);

  Value();
  ~Value();

  Value(const Value& other);
  Value& operator=(const Value& other);
  Value(Value&& other);
  Value& operator=(Value&& other);

  ValueType GetType() const { return type_; }
  ArrayType GetArrayType() const { return elem_type_; }
  bool IsUndefined() const { return type_ == ValueType::kUndefined; }
  bool IsNull() const { return type_ == ValueType::kNull; }
  bool IsEmpty() const { return type_ == ValueType::kEmpty; }

  template <typename T>
  T* Data() {
    T* data = std::get_if<T>(&data_);
    BINDING_DCHECK(data);
    return data;
  }

  template <typename T>
  const T* Data() const {
    const T* data = std::get_if<T>(&data_);
    BINDING_DCHECK(data);
    return data;
  }

  struct ArrayBufferData {
    ArrayBufferData(size_t size, void* data, Finalizer finalizer);
    ~ArrayBufferData();
    ArrayBufferData(const ArrayBufferData& other);
    ArrayBufferData& operator=(const ArrayBufferData& other);
    ArrayBufferData(ArrayBufferData&& other);
    ArrayBufferData& operator=(ArrayBufferData&& other);
    void CopyFrom(const ArrayBufferData& other);
    void MoveFrom(ArrayBufferData&& other);

    size_t size_ = 0;
    void* data_ = nullptr;
    Finalizer finalizer_ = nullptr;
  };

  struct DictionaryData {
    std::vector<std::pair<std::string, Value>> kv;
    int type = kDictionaryInvalidType;
  };

  std::string ToJSONString() const;

 private:
  explicit Value(ValueType t) : type_(t) {}
  explicit Value(bool b) : type_(ValueType::kBoolean) { data_ = b; }
  explicit Value(double num) : type_(ValueType::kNumber) { data_ = num; }
  explicit Value(std::string str)
      : type_(ValueType::kString), data_(std::move(str)) {}
  Value(std::vector<int32_t> array, ValueType type, ArrayType elem_type)
      : type_(type), elem_type_(elem_type), data_(std::move(array)) {}
  Value(std::vector<uint32_t> array, ValueType type, ArrayType elem_type)
      : type_(type), elem_type_(elem_type), data_(std::move(array)) {}
  Value(std::vector<float> array, ValueType type, ArrayType elem_type)
      : type_(type), elem_type_(elem_type), data_(std::move(array)) {}
  Value(std::vector<double> array, ValueType type, ArrayType elem_type)
      : type_(type), elem_type_(elem_type), data_(std::move(array)) {}
  explicit Value(std::vector<std::string> array)
      : type_(ValueType::kArray),
        elem_type_(ArrayType::kTypeString),
        data_(std::move(array)) {}
  explicit Value(std::vector<class Object> array)
      : type_(ValueType::kArray),
        elem_type_(ArrayType::kTypeObject),
        data_(std::move(array)) {}
  explicit Value(std::vector<Value> array)
      : type_(ValueType::kArray),
        elem_type_(ArrayType::kTypeValue),
        data_(std::move(array)) {}
  Value(std::vector<char> data, ArrayType type)
      : type_(ValueType::kArrayBufferView),
        elem_type_(type),
        data_(std::move(data)) {}
  Value(size_t size, void* external_data, Finalizer finalizer)
      : type_(ValueType::kArrayBuffer),
        data_(ArrayBufferData{size, external_data, finalizer}) {}
  explicit Value(class Object&& obj)
      : type_(ValueType::kObject), data_(std::move(obj)) {}
  explicit Value(DictionaryData dict)
      : type_(ValueType::kDictionary), data_(std::move(dict)) {}

  ValueType type_ = ValueType::kEmpty;
  ArrayType elem_type_ = ArrayType::kTypeEmpty;

  std::variant<bool, double, std::string, std::vector<int32_t>,
               std::vector<uint32_t>, std::vector<float>, std::vector<double>,
               std::vector<char>, std::vector<std::string>, DictionaryData,
               ArrayBufferData, class Object, std::vector<class Object>,
               std::vector<Value>>
      data_;
};

}  // namespace binding
}  // namespace lynx

#endif  // BINDING_COMMON_VALUE_H_
