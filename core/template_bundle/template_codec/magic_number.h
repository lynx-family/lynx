// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_MAGIC_NUMBER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_MAGIC_NUMBER_H_

#include <cstdint>
#include <cstring>
#include <vector>

namespace lynx {
namespace template_codec {

// Define constants
extern const uint32_t kQuickBinaryMagic;
extern const uint32_t kLepusBinaryMagic;
extern const uint32_t kRTSBinaryMagic;
extern const uint32_t kRTSNativeBinaryMagic;
extern const uint32_t kTasmSsrSuffixMagic;
extern const uint32_t kLepusBinaryVersion;

// Recognizes a binary format marker without checking decoder support.
bool IsBinaryMagic(uint32_t magic);

// Validates only the total-length field and format marker, without allocation
// or decoding. The decoder still checks the body and runtime compatibility.
inline bool IsValidBinary(const std::vector<uint8_t>& data) {
  uint32_t total_size;
  uint32_t magic;
  if (data.size() < sizeof(total_size) + sizeof(magic)) {
    return false;
  }
  // Use the same field order and byte order as the binary reader.
  std::memcpy(&total_size, data.data(), sizeof(total_size));
  if (total_size != data.size()) {
    return false;
  }
  std::memcpy(&magic, data.data() + sizeof(total_size), sizeof(magic));
  return IsBinaryMagic(magic);
}

}  // namespace template_codec
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_MAGIC_NUMBER_H_
