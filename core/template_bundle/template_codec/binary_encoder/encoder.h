// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_ENCODER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_ENCODER_H_
#include <string>
#include <vector>

#include "core/template_bundle/template_codec/generator/base_struct.h"

namespace lynx {
namespace tasm {
enum EncodeSSRError {
  ERR_MIX_DATA = 101,
  ERR_DECODE,
  ERR_NOT_SSR,
  ERR_BUF,
  ERR_DATA_EMPTY
};

lynx::tasm::EncodeResult encode(const std::string& options_str);
std::string quickjsCheck(const std::string& source);
lynx::tasm::EncodeResult encode_ssr(const uint8_t* ptr, size_t buf_len,
                                    const std::string& mixin_data);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_ENCODER_H_
