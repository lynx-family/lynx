// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/value/byte_array.h"

namespace lynx {
namespace lepus {

ByteArray::ByteArray(std::unique_ptr<uint8_t[]> ptr, size_t length)
    : ptr_(std::move(ptr)), length_(length) {}

std::unique_ptr<uint8_t[]> ByteArray::MovePtr() {
  length_ = 0;
  return std::move(ptr_);
}

uint8_t* ByteArray::GetPtr() { return ptr_.get(); }

size_t ByteArray::GetLength() { return length_; }

}  // namespace lepus
}  // namespace lynx
