// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_COMPRESS_TEMPLATE_COMPRESS_H_
#define CORE_TEMPLATE_BUNDLE_COMPRESS_TEMPLATE_COMPRESS_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/runtime/lepus/binary_input_stream.h"

namespace lynx {
namespace tasm {

bool IsWholeCompressedTemplate(const uint8_t* data, size_t length);

std::vector<uint8_t> WrapWholeCompressedTemplate(
    const uint8_t* data, size_t length, uint32_t requested_chunk_count);

bool UnwrapWholeCompressedTemplate(const uint8_t* data, size_t length,
                                   std::vector<uint8_t>& output,
                                   std::string* error_message);

std::pair<bool, std::unique_ptr<lepus::InputStream>>
MaybeDecompressWholeCompressedTemplateStream(const lepus::InputStream* stream,
                                             std::string& error_message);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_COMPRESS_TEMPLATE_COMPRESS_H_
