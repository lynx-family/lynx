// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <emscripten/bind.h>

#include "core/template_bundle/template_codec/binary_encoder/encoder.h"
#include "core/template_bundle/template_codec/public/tasm_codec.h"

extern "C" {
// TODO(nihao.royal): it seems like a emcc issue that you must export a
// function. update emcc to fix it later.
void quickjsCheck(const char* source) {
  lynx::tasm::quickjsCheck(std::string(source)).c_str();
}
}

namespace {

lynx::tasm::codec::DecodeResult DecodeForWasm(uintptr_t binary_ptr,
                                              size_t length) {
  auto res = lynx::tasm::codec::Decode(
      reinterpret_cast<const uint8_t*>(binary_ptr), length);
  if (res.status != 0) {
    // Preserve the historical wasm contract where decode errors are exposed
    // via `result` instead of `error_msg`.
    res.result = res.error_msg;
  }
  return res;
}

}  // namespace

EMSCRIPTEN_BINDINGS(encode) {
  emscripten::register_vector<uint8_t>("VectorUInt8");
  emscripten::value_object<lynx::tasm::codec::EncodeResult>("EncodeResult")
      .field("status", &lynx::tasm::codec::EncodeResult::status)
      .field("error_msg", &lynx::tasm::codec::EncodeResult::error_msg)
      .field("buffer", &lynx::tasm::codec::EncodeResult::buffer)
      .field("lepus_code", &lynx::tasm::codec::EncodeResult::lepus_code)
      .field("lepus_debug", &lynx::tasm::codec::EncodeResult::lepus_debug)
      .field("section_size", &lynx::tasm::codec::EncodeResult::section_size)
      .field("css_diagnostics",
             &lynx::tasm::codec::EncodeResult::css_diagnostics);
  emscripten::value_object<lynx::tasm::codec::DecodeResult>("DecodeResult")
      .field("status", &lynx::tasm::codec::DecodeResult::status)
      .field("result", &lynx::tasm::codec::DecodeResult::result)
      .field("error_msg", &lynx::tasm::codec::DecodeResult::error_msg);
  function("_encode",
           emscripten::optional_override([](const std::string& options_str) {
             return lynx::tasm::codec::Encode(options_str);
           }),
           emscripten::allow_raw_pointers());
  function("_quickjsCheck", &lynx::tasm::quickjsCheck,
           emscripten::allow_raw_pointers());
  function("_encode_ssr",
           emscripten::optional_override(
               [](intptr_t buf, size_t size, const std::string& data) {
                 return lynx::tasm::encode_ssr(
                     reinterpret_cast<const uint8_t*>(buf), size, data);
               }),
           emscripten::allow_raw_pointers());

  function(
      "_decode",
      emscripten::optional_override([](uintptr_t binaryPtr, size_t length) {
        return DecodeForWasm(binaryPtr, length);
      }),
      emscripten::allow_raw_pointers());
}
