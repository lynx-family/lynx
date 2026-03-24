// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/public/tasm_codec.h"

#include <cstdlib>
#include <cstring>

#include "core/template_bundle/lynx_template_bundle_converter.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "core/template_bundle/template_codec/binary_encoder/encoder.h"
#include "core/template_bundle/template_codec/public/tasm_codec_capi.h"

namespace {

// Helper to copy std::string to char* (caller must free)
char* CopyString(const std::string& str) {
  if (str.empty()) {
    return nullptr;
  }
  char* result = static_cast<char*>(malloc(str.size() + 1));
  if (result) {
    memcpy(result, str.c_str(), str.size() + 1);
  }
  return result;
}

// Helper to copy vector<uint8_t> to uint8_t* (caller must free)
uint8_t* CopyBuffer(const std::vector<uint8_t>& vec, size_t* out_size) {
  if (vec.empty()) {
    *out_size = 0;
    return nullptr;
  }
  uint8_t* result = static_cast<uint8_t*>(malloc(vec.size()));
  if (result) {
    memcpy(result, vec.data(), vec.size());
    *out_size = vec.size();
  }
  return result;
}

}  // namespace

// ============================================================================
// C API Implementation
// ============================================================================

extern "C" {

TasmEncodeResult* Tasm_Encode(const char* options_json) {
  TasmEncodeResult* result =
      static_cast<TasmEncodeResult*>(malloc(sizeof(TasmEncodeResult)));
  if (!result) {
    return nullptr;
  }
  memset(result, 0, sizeof(TasmEncodeResult));

  if (options_json == nullptr) {
    result->status = -1;
    result->error_msg = CopyString("options_json is null");
    return result;
  }

  std::string json_str(options_json);
  lynx::tasm::codec::EncodeResult cpp_result =
      lynx::tasm::codec::Encode(json_str);

  result->status = cpp_result.status;
  result->error_msg = CopyString(cpp_result.error_msg);
  result->buffer = CopyBuffer(cpp_result.buffer, &result->buffer_size);
  result->lepus_code = CopyString(cpp_result.lepus_code);
  result->lepus_debug = CopyString(cpp_result.lepus_debug);
  result->section_size = CopyString(cpp_result.section_size);

  return result;
}

TasmDecodeResult* Tasm_Decode(const uint8_t* data, size_t len) {
  TasmDecodeResult* result =
      static_cast<TasmDecodeResult*>(malloc(sizeof(TasmDecodeResult)));
  if (!result) {
    return nullptr;
  }
  memset(result, 0, sizeof(TasmDecodeResult));

  if (data == nullptr || len == 0) {
    result->status = -1;
    result->error_msg = CopyString("Invalid buffer");
    return result;
  }

  lynx::tasm::codec::DecodeResult cpp_result =
      lynx::tasm::codec::Decode(data, len);

  result->status = cpp_result.status;
  result->result = CopyString(cpp_result.result);
  result->error_msg = CopyString(cpp_result.error_msg);

  return result;
}

void Tasm_FreeEncodeResult(TasmEncodeResult* result) {
  if (result == nullptr) {
    return;
  }
  if (result->error_msg) {
    free(result->error_msg);
  }
  if (result->buffer) {
    free(result->buffer);
  }
  if (result->lepus_code) {
    free(result->lepus_code);
  }
  if (result->lepus_debug) {
    free(result->lepus_debug);
  }
  if (result->section_size) {
    free(result->section_size);
  }
  free(result);
}

void Tasm_FreeDecodeResult(TasmDecodeResult* result) {
  if (result == nullptr) {
    return;
  }
  if (result->result) {
    free(result->result);
  }
  if (result->error_msg) {
    free(result->error_msg);
  }
  free(result);
}

}  // extern "C"

// ============================================================================
// C++ API Implementation
// ============================================================================

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
