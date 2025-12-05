// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/recorder/test_bench_utils.h"

#include <gtest/gtest.h>
#include <zlib.h>

#include <cstring>
#include <string>
#include <vector>

namespace lynx {
namespace devtool {
namespace test {

class TestBenchUtilsTest : public testing::Test {
 protected:
  std::unique_ptr<Byte[]> Compress(const char* source, size_t source_size,
                                   unsigned long* compressed_size_in) {
    *compressed_size_in = compressBound(source_size);
    std::unique_ptr<Byte[]> compressed_data =
        std::make_unique<Byte[]>(*compressed_size_in);
    int z_result =
        compress(compressed_data.get(), compressed_size_in,
                 reinterpret_cast<const Bytef*>(source), source_size);
    if (z_result == Z_OK) {
      return compressed_data;
    } else {
      return nullptr;
    }
  }
};

// Test cases for TestBenchDecode
TEST_F(TestBenchUtilsTest, DecodeBase64NormalCase) {
  // Test normal base64 decoding
  std::string encoded = "SGVsbG8gV29ybGQh";
  std::string decoded = TestBenchDecode(encoded);
  EXPECT_EQ(decoded, "Hello World!");
}

TEST_F(TestBenchUtilsTest, DecodeBase64WithPadding) {
  // Test base64 with padding characters
  std::string encoded = "SGVsbG8=";
  std::string decoded = TestBenchDecode(encoded);
  EXPECT_EQ(decoded, "Hello");
}

TEST_F(TestBenchUtilsTest, DecodeBase64EmptyInput) {
  // Test empty input
  std::string encoded = "";
  std::string decoded = TestBenchDecode(encoded);
  EXPECT_TRUE(decoded.empty());
}

TEST_F(TestBenchUtilsTest, DecodeBase64SpecialCharacters) {
  // Test with special characters
  std::string encoded = "JTI1M0MlMjUyQyUyNTI1JTI1M0Q=";
  std::string decoded = TestBenchDecode(encoded);
  EXPECT_EQ(decoded, "%253C%252C%2525%253D");
}

TEST_F(TestBenchUtilsTest, DecompressEmptyInput) {
  // test empty input
  std::vector<uint8_t> empty_input;
  std::vector<uint8_t> result = TestBenchDecompress(empty_input);

  EXPECT_TRUE(result.empty());
}

// Test cases for TestBenchDecompress
TEST_F(TestBenchUtilsTest, DecompressWithCompressFunction) {
  // test normal compress and decompress
  const std::string original = "Test decompression with Compress function";
  unsigned long compressed_size;
  std::unique_ptr<Byte[]> compressed_data =
      Compress(original.data(), original.size(), &compressed_size);
  ASSERT_NE(compressed_data, nullptr) << "Compression failed";

  std::vector<uint8_t> compressed_vec;
  compressed_vec.reserve(compressed_size);
  for (size_t i = 0; i < compressed_size; ++i) {
    compressed_vec.push_back(compressed_data[i]);
  }

  std::vector<uint8_t> decompressed = TestBenchDecompress(compressed_vec);
  ASSERT_EQ(decompressed.size(), original.size())
      << "Decompressed size mismatch";
  EXPECT_EQ(std::string(decompressed.begin(), decompressed.end()), original)
      << "Decompressed content mismatch";
}

TEST_F(TestBenchUtilsTest, DecompressEmptyData) {
  // test empty data
  const std::string original = "";
  unsigned long compressed_size;
  std::unique_ptr<Byte[]> compressed_data =
      Compress(original.data(), original.size(), &compressed_size);
  ASSERT_NE(compressed_data, nullptr) << "Compression failed for empty data";

  std::vector<uint8_t> compressed_vec(compressed_data.get(),
                                      compressed_data.get() + compressed_size);
  std::vector<uint8_t> decompressed = TestBenchDecompress(compressed_vec);

  EXPECT_TRUE(decompressed.empty())
      << "Decompressed empty data should be empty";
}

TEST_F(TestBenchUtilsTest, DecompressInvalidData) {
  // test invalid data
  std::vector<uint8_t> invalid_data = {
      0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  // invalid gzip header
  std::vector<uint8_t> decompressed = TestBenchDecompress(invalid_data);

  EXPECT_TRUE(decompressed.empty())
      << "Decompress should return empty for invalid data";
}

TEST_F(TestBenchUtilsTest, DecompressLargeData) {
  // test large data
  std::string original;
  for (int i = 0; i < 1024; ++i) {
    original += "abcdefghijklmnopqrstuvwxyz";
  }
  unsigned long compressed_size;
  std::unique_ptr<Byte[]> compressed_data =
      Compress(original.data(), original.size(), &compressed_size);
  ASSERT_NE(compressed_data, nullptr) << "Compression failed for large data";

  std::vector<uint8_t> compressed_vec(compressed_data.get(),
                                      compressed_data.get() + compressed_size);
  std::vector<uint8_t> decompressed = TestBenchDecompress(compressed_vec);

  ASSERT_EQ(decompressed.size(), original.size())
      << "Decompressed size mismatch for large data";
  EXPECT_EQ(std::string(decompressed.begin(), decompressed.end()), original)
      << "Decompressed content mismatch for large data";
}

}  // namespace test
}  // namespace devtool
}  // namespace lynx
