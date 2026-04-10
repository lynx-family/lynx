// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_TYPES_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_TYPES_H_

#include <string>
#include <vector>

namespace lynx {
namespace tasm {

struct EncodeResult {
  int status = -1;
  std::string error_msg;
  std::vector<uint8_t> buffer;
  std::string lepus_code;
  std::string lepus_debug;
  std::string section_size;
  std::string css_diagnostics;
};

struct DecodeResult {
  int status = -1;
  std::string result;
  std::string error_msg;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_PUBLIC_TASM_CODEC_TYPES_H_
