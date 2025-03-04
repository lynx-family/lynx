// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_MAGIC_NUMBER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_MAGIC_NUMBER_H_

#include <cstdint>

namespace lynx {
namespace template_codec {

// Define constants
extern const uint32_t kQuickBinaryMagic;
extern const uint32_t kLepusBinaryMagic;
extern const uint32_t kTasmSsrSuffixMagic;
extern const uint32_t kLepusBinaryVersion;

}  // namespace template_codec
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_MAGIC_NUMBER_H_
