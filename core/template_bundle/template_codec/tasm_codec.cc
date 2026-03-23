// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/public/tasm_codec.h"

#include <string>
#include <vector>

#include "core/template_bundle/lynx_template_bundle_converter.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "core/template_bundle/template_codec/binary_encoder/encoder.h"

namespace lynx {
namespace tasm {
namespace codec {

EncodeResult Encode(const std::string& options_json) {
  return lynx::tasm::encode(options_json);
}

DecodeResult Decode(const uint8_t* data, size_t len) {
  DecodeResult out;
  out.status = -1;
  if (data == nullptr || len == 0) {
    out.error_msg = "Invalid Buffer!";
    return out;
  }

  auto reader = lynx::tasm::LynxBinaryReader::CreateLynxBinaryReader(
      std::vector<uint8_t>(data, data + len));
  if (!reader.Decode()) {
    out.error_msg = reader.error_message_;
    return out;
  }

  auto template_bundle = reader.GetTemplateBundle();
  out.status = 0;
  out.result = lynx::tasm::LynxTemplateBundleConverter::
      ConvertTemplateBundleToSerializedString(template_bundle);
  return out;
}

}  // namespace codec
}  // namespace tasm
}  // namespace lynx
