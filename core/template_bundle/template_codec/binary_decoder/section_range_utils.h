// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_SECTION_RANGE_UTILS_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_SECTION_RANGE_UTILS_H_

#include <cstdint>
#include <limits>

#include "core/runtime/lepus/binary_input_stream.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace tasm {

// Validates that the section starting at the stream's current offset and
// spanning section_length bytes is contained in the stream and representable
// by Range's uint32_t offsets. Updates section_range only when validation
// succeeds.
inline bool ValidateAndSetSectionRange(const lepus::InputStream& stream,
                                       uint32_t section_length,
                                       Range& section_range) {
  const size_t section_start = stream.offset();
  if (section_start > stream.size() ||
      section_start > std::numeric_limits<uint32_t>::max() ||
      section_length > stream.size() - section_start ||
      section_length > std::numeric_limits<uint32_t>::max() - section_start) {
    return false;
  }
  section_range.start = static_cast<uint32_t>(section_start);
  section_range.end = static_cast<uint32_t>(section_start + section_length);
  return true;
}

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_SECTION_RANGE_UTILS_H_
