// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/datauri_utils.h"

#include "base/include/string/string_utils.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace base {
namespace {
const constexpr char *kDataURIPrefix = "data:";
const constexpr char *kBase64Prefix = ";base64,";
}  // namespace

bool DataURIUtil::IsDataURI(const std::string_view &uri) {
  return BeginsWith(uri, kDataURIPrefix);
}

int32_t DataURIUtil::DecodeBase64(
    const std::string_view &base64_str,
    lynx::base::DataURIUtil::BufferFactory factory) {
  size_t buffer_size = modp_b64_decode_len(base64_str.length());
  auto buffer = factory(buffer_size);
  size_t actual_buffer_size =
      modp_b64_decode(buffer, base64_str.data(), base64_str.length());
  if (actual_buffer_size == static_cast<size_t>(-1)) {
    return 0;
  }
  return actual_buffer_size;
}

int32_t DataURIUtil::DecodeDataURI(
    const std::string_view &uri,
    lynx::base::DataURIUtil::BufferFactory factory) {
  size_t pos = uri.find(kBase64Prefix);
  if (pos == std::string_view::npos) {
    return 0;
  }

  return DecodeBase64(uri.substr(pos + sizeof(kBase64Prefix)),
                      std::move(factory));
}

}  // namespace base
}  // namespace lynx
