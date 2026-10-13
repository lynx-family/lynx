// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/memory_monitor/testing/memory_record_unittest.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace lynx {
namespace tasm {
namespace test {

void MemoryRecordTest::SetUp() {
  empty_record_ = performance::MemoryRecord();
  basic_record_ =
      performance::MemoryRecord(performance::kCategoryMTSEngine, 1024.0f);
  detailed_record_ = performance::MemoryRecord(
      performance::kCategoryBTSEngine, 2048.0f,
      std::make_unique<std::unordered_map<std::string, std::string>>(
          std::unordered_map<std::string, std::string>{{"url", "test.com"},
                                                       {"id", "123"}}));
}

// Tests the += operator functionality
TEST_F(MemoryRecordTest, PlusEqualsOperator) {
  // Adding same category
  performance::MemoryRecord r1(performance::kCategoryMTSEngine, 512.0f);
  performance::MemoryRecord r2(performance::kCategoryMTSEngine, 256.0f);
  r1 += r2;
  EXPECT_FLOAT_EQ(r1.size_bytes_, 768.0f);

  // Adding different categories (should this be allowed? verify business logic)
  performance::MemoryRecord r3(performance::kCategoryBTSEngine, 128.0f);
  r1 += r3;
  EXPECT_FLOAT_EQ(r1.size_bytes_, 896.0f);

  // Adding with details
  performance::MemoryRecord r4(
      performance::kCategoryBTSEngine, 64.0f,
      std::make_unique<std::unordered_map<std::string, std::string>>(
          std::unordered_map<std::string, std::string>{{"key1", "value1"}}));
  performance::MemoryRecord r5(
      performance::kCategoryBTSEngine, 32.0f,
      std::make_unique<std::unordered_map<std::string, std::string>>(
          std::unordered_map<std::string, std::string>{{"key2", "value2"}}));
  r4 += r5;
  EXPECT_FLOAT_EQ(r4.size_bytes_, 96.0f);
  ASSERT_TRUE(r4.detail_ != nullptr);
  EXPECT_EQ(r4.detail_->size(), 2);
  EXPECT_EQ(r4.detail_->at("key1"), "value1");
  EXPECT_EQ(r4.detail_->at("key2"), "value2");

  // Adding with duplicate keys (later values should overwrite)
  performance::MemoryRecord r6(
      performance::kCategoryBTSEngine, 16.0f,
      std::make_unique<std::unordered_map<std::string, std::string>>(
          std::unordered_map<std::string, std::string>{{"key", "old"}}));
  performance::MemoryRecord r7(
      performance::kCategoryBTSEngine, 8.0f,
      std::make_unique<std::unordered_map<std::string, std::string>>(
          std::unordered_map<std::string, std::string>{{"key", "new"}}));
  r6 += r7;
  EXPECT_EQ(r6.detail_->at("key"), "new");
}

// Tests the -= operator functionality
TEST_F(MemoryRecordTest, MinusEqualsOperator) {
  // Basic subtraction
  performance::MemoryRecord r1(performance::kCategoryMTSEngine, 1024.0f);
  performance::MemoryRecord r2(performance::kCategoryMTSEngine, 512.0f);
  r1 -= r2;
  EXPECT_FLOAT_EQ(r1.size_bytes_, 512.0f);

  // Subtracting larger value (should negative be allowed? verify business
  // logic)
  performance::MemoryRecord r3(performance::kCategoryMTSEngine, 256.0f);
  r1 -= r3;
  EXPECT_FLOAT_EQ(r1.size_bytes_, 256.0f);
  r1 -= performance::MemoryRecord(performance::kCategoryMTSEngine, 512.0f);
  EXPECT_FLOAT_EQ(r1.size_bytes_, -256.0f);

  // Subtracting with details
  performance::MemoryRecord r4(
      performance::kCategoryBTSEngine, 128.0f,
      std::make_unique<std::unordered_map<std::string, std::string>>(
          std::unordered_map<std::string, std::string>{{"key1", "value1"},
                                                       {"key2", "value2"}}));
  performance::MemoryRecord r5(
      performance::kCategoryBTSEngine, 64.0f,
      std::make_unique<std::unordered_map<std::string, std::string>>(
          std::unordered_map<std::string, std::string>{{"key1", "value1"}}));
  r4 -= r5;
  EXPECT_FLOAT_EQ(r4.size_bytes_, 64.0f);
  ASSERT_TRUE(r4.detail_ != nullptr);
  EXPECT_EQ(r4.detail_->size(), 1);
  EXPECT_EQ(r4.detail_->at("key2"), "value2");
}

// Tests edge cases and boundary conditions
TEST_F(MemoryRecordTest, EdgeCases) {
  // Zero value tests
  performance::MemoryRecord zero(performance::kCategoryMTSEngine, 0.0f);
  zero += performance::MemoryRecord(performance::kCategoryMTSEngine, 0.0f);
  EXPECT_FLOAT_EQ(zero.size_bytes_, 0.0f);

  // Negative value tests (should these be allowed?)
  performance::MemoryRecord negative(performance::kCategoryMTSEngine, -1024.0f);
  negative +=
      performance::MemoryRecord(performance::kCategoryMTSEngine, 512.0f);
  EXPECT_FLOAT_EQ(negative.size_bytes_, -512.0f);

  // Operations with empty detail
  performance::MemoryRecord no_detail(performance::kCategoryMTSEngine, 100.0f);
  performance::MemoryRecord with_detail(
      performance::kCategoryMTSEngine, 50.0f,
      std::make_unique<std::unordered_map<std::string, std::string>>(
          std::unordered_map<std::string, std::string>{{"a", "b"}}));
  no_detail += with_detail;
  ASSERT_TRUE(no_detail.detail_ != nullptr);
  EXPECT_EQ(no_detail.detail_->size(), 1);

  with_detail -= no_detail;
  EXPECT_FLOAT_EQ(with_detail.size_bytes_, -100.0f);
  EXPECT_TRUE(with_detail.detail_->empty());
}

// Tests move semantics
TEST_F(MemoryRecordTest, MoveSemantics) {
  performance::MemoryRecord source(
      performance::kCategoryTasmElement, 512.0f,
      std::make_unique<std::unordered_map<std::string, std::string>>(
          std::unordered_map<std::string, std::string>{{"id", "elem1"}}));

  performance::MemoryRecord moved_to = std::move(source);

  EXPECT_EQ(moved_to.category_, performance::kCategoryTasmElement);
  EXPECT_FLOAT_EQ(moved_to.size_bytes_, 512.0f);
  ASSERT_TRUE(moved_to.detail_ != nullptr);
  EXPECT_EQ(moved_to.detail_->at("id"), "elem1");

  // Source object should be in valid but unspecified state after move
  EXPECT_TRUE(source.category_.empty());  // Actual implementation may vary
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
