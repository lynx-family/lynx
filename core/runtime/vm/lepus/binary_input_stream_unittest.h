// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_BINARY_INPUT_STREAM_UNITTEST_H_
#define CORE_RUNTIME_VM_LEPUS_BINARY_INPUT_STREAM_UNITTEST_H_

#include "core/runtime/vm/lepus/binary_input_stream.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace lepus {
namespace test {

class ByteArrayInputStreamTest : public ::testing::Test {
 public:
  ByteArrayInputStreamTest() = default;
  ~ByteArrayInputStreamTest() = default;

  void SetUp() override {}

  void TearDown() override {}
};

}  // namespace test
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_BINARY_INPUT_STREAM_UNITTEST_H_
