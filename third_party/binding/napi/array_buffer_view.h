// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BINDING_NAPI_ARRAY_BUFFER_VIEW_H_
#define BINDING_NAPI_ARRAY_BUFFER_VIEW_H_

#include "third_party/binding/napi/shim/shim_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace binding {

// Data is not owned. Copy the buffer if future use is expected.
class ArrayBufferView {
 public:
  static ArrayBufferView From(const Napi::TypedArray& typed_array) {
    return ArrayBufferView(typed_array);
  }
  static ArrayBufferView From(const Napi::DataView& data_view) {
    return ArrayBufferView(data_view);
  }

  ArrayBufferView(const Napi::TypedArray& typed_array)
      : buffer_(typed_array.ArrayBuffer()),
        length_(typed_array.ByteLength()),
        offset_(typed_array.ByteOffset()),
        size_(typed_array.ElementSize()) {
    switch (typed_array.TypedArrayType()) {
      case napi_int8_array:
        type_ = kTypeInt8;
        break;
      case napi_uint8_array:
        type_ = kTypeUint8;
        break;
      case napi_uint8_clamped_array:
        type_ = kTypeUint8Clamped;
        break;
      case napi_int16_array:
        type_ = kTypeInt16;
        break;
      case napi_uint16_array:
        type_ = kTypeUint16;
        break;
      case napi_int32_array:
        type_ = kTypeInt32;
        break;
      case napi_uint32_array:
        type_ = kTypeUint32;
        break;
      case napi_float32_array:
        type_ = kTypeFloat32;
        break;
      case napi_float64_array:
        type_ = kTypeFloat64;
        break;
      case napi_bigint64_array:
        type_ = kTypeBigInt64;
        break;
      case napi_biguint64_array:
        type_ = kTypeBigUint64;
        break;
      default:
        break;
    }
  }
  explicit ArrayBufferView(const Napi::DataView& data_view)
      : buffer_(data_view.ArrayBuffer()),
        type_(kTypeDataView),
        length_(data_view.ByteLength()),
        offset_(data_view.ByteOffset()) {}
  ArrayBufferView() : type_(kTypeEmpty) {}

  enum ViewType {
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
    kTypeDataView
  };

  ArrayBufferView(void* data, ViewType type, size_t length)
      : data_(data), native_initialized_(true), type_(type), length_(length) {
    switch (type_) {
      case kTypeInt8:
      case kTypeUint8:
      case kTypeUint8Clamped:
        size_ = 1;
        break;
      case kTypeInt16:
      case kTypeUint16:
        size_ = 2;
        break;
      case kTypeInt32:
      case kTypeUint32:
      case kTypeFloat32:
        size_ = 4;
        break;
      case kTypeFloat64:
      case kTypeBigInt64:
      case kTypeBigUint64:
        size_ = 8;
        break;
      default:
        size_ = 0;
        break;
    }
  }

  ViewType GetType() { return type_; }

  // Gets a pointer to the data buffer.
  void* Data() {
    if (native_initialized_) return data_;
    return reinterpret_cast<uint8_t*>(buffer_.Data()) + offset_;
  }

  // Gets the length of the array buffer in bytes.
  size_t ByteLength() { return length_; }

  bool IsUint8Array() const { return type_ == kTypeUint8; }

  bool IsUint8ClampedArray() const { return type_ == kTypeUint8Clamped; }

  bool IsInt8Array() const { return type_ == kTypeInt8; }

  bool IsUint16Array() const { return type_ == kTypeUint16; }

  bool IsInt16Array() const { return type_ == kTypeInt16; }

  bool IsUint32Array() const { return type_ == kTypeUint32; }

  bool IsInt32Array() const { return type_ == kTypeInt32; }

  bool IsFloat32Array() const { return type_ == kTypeFloat32; }

  bool IsFloat64Array() const { return type_ == kTypeFloat64; }

  bool IsEmpty() const { return type_ == kTypeEmpty; }

  uint8_t GetTypeSize() {
    if (type_ == kTypeEmpty || type_ == kTypeDataView) {
      return 0;
    }
    return size_;
  }

 private:
  Napi::ArrayBuffer buffer_;
  void* data_ = nullptr;
  bool native_initialized_ = false;
  ViewType type_;
  size_t length_;
  size_t offset_ = 0;
  uint8_t size_ = 0;
};

}  // namespace binding
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // BINDING_NAPI_ARRAY_BUFFER_VIEW_H_
