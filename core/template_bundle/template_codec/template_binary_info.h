// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_TEMPLATE_BINARY_INFO_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_TEMPLATE_BINARY_INFO_H_

#include <cstddef>
#include <cstdint>

#include "core/template_bundle/template_codec/public/tasm_codec_types.h"

namespace lynx {
namespace tasm {
namespace codec {

DecodeResult DecodeTemplateBinaryInfoImpl(const uint8_t* data, size_t len);

}  // namespace codec
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_TEMPLATE_BINARY_INFO_H_
