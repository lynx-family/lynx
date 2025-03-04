// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/common/value.h"

#include <cstring>

namespace lynx {
namespace binding {

// static
Value Value::Null() { return Value(ValueType::kNull); }

// static
Value Value::Undefined() { return Value(ValueType::kUndefined); }

// static
Value Value::Boolean(bool b) { return Value(b); }

// static
Value Value::Number(double num) { return Value(num); }

// static
Value Value::String(std::string str) { return Value(std::move(str)); }

// static
Value Value::Array(std::vector<int32_t> array, ArrayType type) {
  BINDING_DCHECK(type == ArrayType::kTypeBoolean ||
                 type == ArrayType::kTypeInt32);
  return Value(std::move(array), ValueType::kArray, type);
}

// static
Value Value::Array(std::vector<uint32_t> array, ArrayType type) {
  BINDING_DCHECK(type == ArrayType::kTypeUint32);
  return Value(std::move(array), ValueType::kArray, type);
}

// static
Value Value::Array(std::vector<float> array, ArrayType type) {
  BINDING_DCHECK(type == ArrayType::kTypeFloat32);
  return Value(std::move(array), ValueType::kArray, type);
}

// static
Value Value::Array(std::vector<double> array, ArrayType type) {
  BINDING_DCHECK(type == ArrayType::kTypeFloat64);
  return Value(std::move(array), ValueType::kArray, type);
}

// static
Value Value::Array(std::vector<std::string> array) {
  return Value(std::move(array));
}

// static
Value Value::Array(std::vector<class Object> array) {
  return Value(std::move(array));
}

// static
Value Value::Array(std::vector<Value> array) { return Value(std::move(array)); }

// static
Value Value::Int32Array(std::vector<int32_t> array) {
  return Value(std::move(array), ValueType::kTypedArray, ArrayType::kTypeInt32);
}

// static
Value Value::Uint32Array(std::vector<uint32_t> array) {
  return Value(std::move(array), ValueType::kTypedArray,
               ArrayType::kTypeUint32);
}

// static
Value Value::Float32Array(std::vector<float> array) {
  return Value(std::move(array), ValueType::kTypedArray,
               ArrayType::kTypeFloat32);
}

// static
Value Value::ArrayBufferView(std::vector<char> data, ArrayType type) {
  return Value(std::move(data), type);
}

// static
Value Value::ArrayBuffer(size_t size, void* data, Finalizer finalizer) {
  return Value(size, data, finalizer);
}

// static
Value Value::Object(class Object&& obj) { return Value(std::move(obj)); }

// static
Value Value::Dictionary(std::vector<std::pair<std::string, Value>> dict,
                        int type) {
  return Value({std::move(dict), type});
}

Value::Value() = default;

Value::~Value() = default;

Value::Value(const Value& other) = default;

Value& Value::operator=(const Value& other) = default;

Value::Value(Value&& other) = default;

Value& Value::operator=(Value&& other) = default;

Value::ArrayBufferData::ArrayBufferData(size_t size, void* data,
                                        Finalizer finalizer)
    : size_(size), data_(data), finalizer_(finalizer) {}

Value::ArrayBufferData::~ArrayBufferData() {
  if (finalizer_) {
    finalizer_(data_);
  }
}

void Value::ArrayBufferData::CopyFrom(const ArrayBufferData& other) {
  size_ = other.size_;
  data_ = other.data_;
  finalizer_ = other.finalizer_;
  if (finalizer_) {
    data_ = std::malloc(size_);
    std::memcpy(data_, other.data_, size_);
    finalizer_ = [](void* data) { std::free(data); };
  }
}

void Value::ArrayBufferData::MoveFrom(ArrayBufferData&& other) {
  size_ = other.size_;
  data_ = other.data_;
  finalizer_ = other.finalizer_;
  other.size_ = 0;
  other.data_ = nullptr;
  other.finalizer_ = nullptr;
}

Value::ArrayBufferData::ArrayBufferData(const ArrayBufferData& other) {
  CopyFrom(other);
}

Value::ArrayBufferData& Value::ArrayBufferData::operator=(
    const ArrayBufferData& other) {
  CopyFrom(other);
  return *this;
}

Value::ArrayBufferData::ArrayBufferData(ArrayBufferData&& other) {
  MoveFrom(std::move(other));
}

Value::ArrayBufferData& Value::ArrayBufferData::operator=(
    ArrayBufferData&& other) {
  MoveFrom(std::move(other));
  return *this;
}

}  // namespace binding
}  // namespace lynx
