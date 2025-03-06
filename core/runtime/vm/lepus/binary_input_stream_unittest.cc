// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/runtime/vm/lepus/binary_input_stream_unittest.h"

namespace lynx {
namespace lepus {
namespace test {

TEST_F(ByteArrayInputStreamTest, TestCursor) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  EXPECT_EQ(*stream->cursor(), 't');
}

TEST_F(ByteArrayInputStreamTest, TestCheckSize0) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  EXPECT_TRUE(stream->CheckSize(str.size()));
  EXPECT_FALSE(stream->CheckSize(str.size() + 1));
}

TEST_F(ByteArrayInputStreamTest, TestCheckSize1) {
  std::string str = "";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  EXPECT_FALSE(stream->CheckSize(str.size()));
  EXPECT_FALSE(stream->CheckSize(str.size() + 1));
}

TEST_F(ByteArrayInputStreamTest, TestSeek) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  EXPECT_EQ(stream->Seek(1), 1);
  EXPECT_EQ(stream->offset(), 1);
  EXPECT_EQ(stream->Seek(0), 0);
  EXPECT_EQ(stream->offset(), 0);
  EXPECT_EQ(stream->Seek(str.size()), str.size() - 1);
  EXPECT_EQ(stream->offset(), str.size() - 1);
  EXPECT_EQ(stream->Seek(0), 0);
  EXPECT_EQ(stream->offset(), 0);
  EXPECT_EQ(stream->Seek(str.size() + 1), str.size() - 1);
  EXPECT_EQ(stream->offset(), str.size() - 1);
}

TEST_F(ByteArrayInputStreamTest, TestReadUx0) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  uint8_t result = 0;
  stream->ReadUx<uint8_t>(&result);
  EXPECT_EQ(result, static_cast<uint8_t>('t'));
}

TEST_F(ByteArrayInputStreamTest, TestReadUx1) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  int32_t result = 0;
  stream->ReadUx<int32_t>(&result);
  EXPECT_EQ(result, static_cast<int32_t>('t') + static_cast<int32_t>('e' << 8) +
                        static_cast<int32_t>('s' << 16) +
                        static_cast<int32_t>('t' << 24));
}

TEST_F(ByteArrayInputStreamTest, TestReadData) {}

TEST_F(ByteArrayInputStreamTest, TestReadStringStd0) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  std::string target_str;
  stream->ReadString(target_str, str.size());
  EXPECT_EQ(str, target_str);
}

TEST_F(ByteArrayInputStreamTest, TestReadStringStd1) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  std::string target_str;
  stream->ReadString(target_str, str.size() + 1);
  EXPECT_EQ(target_str, "");

  stream->ReadString(target_str, 2);
  EXPECT_EQ(target_str, "te");

  stream->ReadString(target_str, 2);
  EXPECT_EQ(target_str, "st");
}

TEST_F(ByteArrayInputStreamTest, TestReadString0) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  base::String target_str;
  stream->ReadString(target_str, str.size());
  EXPECT_EQ(target_str, "test string");
}

TEST_F(ByteArrayInputStreamTest, TestReadString1) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  base::String target_str;
  stream->ReadString(target_str, str.size() + 1);
  EXPECT_EQ(target_str, "");

  stream->ReadString(target_str, 2);
  EXPECT_EQ(target_str, "te");

  stream->ReadString(target_str, 2);
  EXPECT_EQ(target_str, "st");
}

TEST_F(ByteArrayInputStreamTest, TestReadCompactU32) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  uint32_t result = 0;
  stream->ReadCompactU32(&result);
  EXPECT_EQ(result, static_cast<uint32_t>('t') +
                        static_cast<uint32_t>('e' << 8) +
                        static_cast<uint32_t>('s' << 16) +
                        static_cast<uint32_t>('t' << 24));
}

TEST_F(ByteArrayInputStreamTest, TestReadCompactS32) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  int32_t result = 0;
  stream->ReadCompactS32(&result);
  EXPECT_EQ(result, static_cast<int32_t>('t') + static_cast<int32_t>('e' << 8) +
                        static_cast<int32_t>('s' << 16) +
                        static_cast<int32_t>('t' << 24));
}

TEST_F(ByteArrayInputStreamTest, TestReadCompactU64) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  uint64_t result = 0;
  stream->ReadCompactU64(&result);
  EXPECT_EQ(result, static_cast<uint64_t>('t') +
                        static_cast<uint64_t>('e' << 8) +
                        static_cast<uint64_t>('s' << 16) +
                        static_cast<uint64_t>('t' << 24) +
                        (static_cast<uint64_t>(' ') << 32) +
                        (static_cast<uint64_t>('s') << 40) +
                        (static_cast<uint64_t>('t') << 48) +
                        (static_cast<uint64_t>('r') << 56));
}

TEST_F(ByteArrayInputStreamTest, TestDeriveInputStream) {
  std::string str = "test string";

  auto stream = std::make_unique<ByteArrayInputStream>(
      reinterpret_cast<const uint8_t*>(str.data()), str.size());

  auto new_stream = stream->DeriveInputStream();

  EXPECT_EQ(static_cast<ByteArrayInputStream*>(new_stream.get())->buf_,
            stream->buf_);
}

}  // namespace test
}  // namespace lepus
}  // namespace lynx
