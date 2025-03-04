// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/magic_number.h"

namespace lynx {
namespace template_codec {

// Define constants
const uint32_t kQuickBinaryMagic = 0x00241922;
const uint32_t kLepusBinaryMagic = 0xdd737199;
const uint32_t kTasmSsrSuffixMagic = 0xa8432251;
const uint32_t kLepusBinaryVersion = 1;

}  // namespace template_codec
}  // namespace lynx
