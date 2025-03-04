// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_BYTE_ARRAY_H_
#define CORE_RUNTIME_VM_LEPUS_BYTE_ARRAY_H_

#include <memory>
#include <utility>

#include "core/runtime/vm/lepus/ref_counted_class.h"
#include "core/runtime/vm/lepus/ref_type.h"

namespace lynx {
namespace lepus {

class ByteArray : public lepus::RefCounted {
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

  virtual void ReleaseSelf() const override;
  ~ByteArray() override = default;

  RefType GetRefType() const override { return RefType::kByteArray; };

 protected:
  ByteArray() = default;
  ByteArray(std::unique_ptr<uint8_t[]> ptr, size_t length);

 private:
  std::unique_ptr<uint8_t[]> ptr_;
  size_t length_{0};
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_BYTE_ARRAY_H_
