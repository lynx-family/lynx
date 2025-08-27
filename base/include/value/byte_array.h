// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_VALUE_BYTE_ARRAY_H_
#define BASE_INCLUDE_VALUE_BYTE_ARRAY_H_

#include <memory>
#include <utility>

#include "base/include/base_export.h"
#include "base/include/value/ref_counted_class.h"
#include "base/include/value/ref_type.h"

namespace lynx {
namespace lepus {

class BASE_EXPORT ByteArray : public lepus::RefCounted {
 public:
  static fml::RefPtr<ByteArray> Create() {
    return fml::AdoptRef<ByteArray>(new ByteArray());
  }
  static fml::RefPtr<ByteArray> Create(std::unique_ptr<uint8_t[]> ptr,
                                       size_t length) {
    return fml::AdoptRef<ByteArray>(new ByteArray(std::move(ptr), length));
  }

  std::unique_ptr<uint8_t[]> MovePtr();

  size_t GetLength();

  uint8_t* GetPtr();

  ~ByteArray() override = default;

  RefType GetRefType() const override { return RefType::kByteArray; };

 protected:
  ByteArray() = default;
  ByteArray(std::unique_ptr<uint8_t[]> ptr, size_t length);

  friend class Value;

  void Reset() {
    ptr_ = nullptr;
    length_ = 0;
  }

 private:
  std::unique_ptr<uint8_t[]> ptr_;
  size_t length_{0};
};

}  // namespace lepus
}  // namespace lynx

#endif  // BASE_INCLUDE_VALUE_BYTE_ARRAY_H_
