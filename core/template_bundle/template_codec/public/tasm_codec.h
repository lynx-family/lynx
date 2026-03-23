// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_H_

#include <cstddef>
#include <cstdint>
#include <string>

#include "core/template_bundle/template_codec/public/tasm_codec_types.h"

namespace lynx {
namespace tasm {
namespace codec {

using EncodeResult = lynx::tasm::EncodeResult;
using DecodeResult = lynx::tasm::DecodeResult;

EncodeResult Encode(const std::string& options_json);
DecodeResult Decode(const uint8_t* data, size_t len);

}  // namespace codec
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_H_
