// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <limits>
#include <string>
#include <vector>

#include "clay/fml/base64.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(Base64Test, RejectsEncodedSizeOverflow) {
  const size_t max_length = std::numeric_limits<size_t>::max();
  EXPECT_DEATH(fml::Base64::Encode(nullptr, max_length, nullptr), "");
  EXPECT_DEATH(fml::Base64::Encode(nullptr, max_length / 4 * 3 + 1, nullptr),
               "");
  char output = 0;
  EXPECT_DEATH(fml::Base64::Encode(nullptr, max_length, &output), "");
}

TEST(Base64Test, ComputesLargestRepresentableEncodedSize) {
  const size_t groups = std::numeric_limits<size_t>::max() / 4;
  EXPECT_EQ(fml::Base64::Encode(nullptr, groups * 3, nullptr), groups * 4);
}

TEST(Base64Test, CanEncodeAndDecode) {
  const std::string original = "abcdefghijklmnopqrstuvwxyz0123456789+/=";
  const std::string encoded =
      "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXowMTIzNDU2Nzg5Ky89";

  size_t encoded_size =
      fml::Base64::Encode(original.c_str(), original.length(), nullptr);
  std::string encoded_result(encoded_size, '\0');
  fml::Base64::Encode(original.c_str(), original.length(),
                      encoded_result.data());

  ASSERT_EQ(encoded, encoded_result);

  size_t decoded_size = 0;
  fml::Base64::Decode(encoded.c_str(), encoded.length(), nullptr,
                      &decoded_size);
  std::string decoded_result(decoded_size, '\0');
  ASSERT_EQ(fml::Base64::kNoError,
            fml::Base64::Decode(encoded.c_str(), encoded.length(),
                                decoded_result.data(), &decoded_size));

  ASSERT_EQ(original, decoded_result);
}

TEST(Base64Test, CanEncodeAndDecodeWithPadding) {
  const std::string original = "pleasure.";
  const std::string encoded = "cGxlYXN1cmUu";

  size_t encoded_size =
      fml::Base64::Encode(original.c_str(), original.length(), nullptr);
  std::string encoded_result(encoded_size, '\0');
  fml::Base64::Encode(original.c_str(), original.length(),
                      encoded_result.data());

  ASSERT_EQ(encoded, encoded_result);

  size_t decoded_size = 0;
  fml::Base64::Decode(encoded.c_str(), encoded.length(), nullptr,
                      &decoded_size);
  std::string decoded_result(decoded_size, '\0');
  ASSERT_EQ(fml::Base64::kNoError,
            fml::Base64::Decode(encoded.c_str(), encoded.length(),
                                decoded_result.data(), &decoded_size));

  ASSERT_EQ(original, decoded_result);
}

TEST(Base64Test, DecodeInvalidString) {
  const std::string invalid_encoded = "invalid~";
  size_t decoded_size = 0;
  ASSERT_EQ(
      fml::Base64::kBadCharError,
      fml::Base64::Decode(invalid_encoded.c_str(), invalid_encoded.length(),
                          nullptr, &decoded_size));
}
