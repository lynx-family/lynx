// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <emscripten/bind.h>

#include "core/template_bundle/template_codec/binary_encoder/encoder.h"
#include "core/template_bundle/template_codec/generator/base_struct.h"
#include "third_party/aes/aes.h"

EMSCRIPTEN_BINDINGS(encode) {
  emscripten::register_vector<uint8_t>("VectorUInt8");
  emscripten::value_object<lynx::tasm::EncodeResult>("EncodeResult")
      .field("status", &lynx::tasm::EncodeResult::status)
      .field("error_msg", &lynx::tasm::EncodeResult::error_msg)
      .field("buffer", &lynx::tasm::EncodeResult::buffer)
      .field("lepus_code", &lynx::tasm::EncodeResult::lepus_code)
      .field("lepus_debug", &lynx::tasm::EncodeResult::lepus_debug)
      .field("section_size", &lynx::tasm::EncodeResult::section_size);
  function("_encode", &lynx::tasm::encode, emscripten::allow_raw_pointers());
  function("_quickjsCheck", &lynx::tasm::quickjsCheck,
           emscripten::allow_raw_pointers());
  function("_encode_ssr",
           emscripten::optional_override(
               [](intptr_t buf, size_t size, const std::string& data) {
                 return lynx::tasm::encode_ssr(
                     reinterpret_cast<const uint8_t*>(buf), size, data);
               }),
           emscripten::allow_raw_pointers());
  function("_encrypt",
           emscripten::optional_override([](const std::string& plain) {
             return AES().EncryptECB(plain);
           }),
           emscripten::allow_raw_pointers());
  function("_decrypt",
           emscripten::optional_override([](const std::string& cipher) {
             return AES().DecryptECB(cipher);
           }),
           emscripten::allow_raw_pointers());
}
