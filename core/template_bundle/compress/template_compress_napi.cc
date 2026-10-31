// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <vector>

#include "core/template_bundle/compress/template_compress.h"
#include "node_api.h"

namespace {

bool GetUint8ArrayArgument(napi_env env, napi_callback_info info,
                           uint8_t** data_out, size_t* size_out,
                           uint32_t* chunk_count_out) {
  size_t argc = 2;
  napi_value argv[2] = {nullptr, nullptr};
  if (napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr) != napi_ok) {
    return false;
  }

  if (argc < 1) {
    napi_throw_type_error(env, nullptr,
                          "compressTemplate expects a Uint8Array");
    return false;
  }

  bool is_typedarray = false;
  if (napi_is_typedarray(env, argv[0], &is_typedarray) != napi_ok ||
      !is_typedarray) {
    napi_throw_type_error(env, nullptr,
                          "compressTemplate expects a Uint8Array");
    return false;
  }

  napi_typedarray_type type = napi_int8_array;
  size_t length = 0;
  void* data = nullptr;
  napi_value array_buffer = nullptr;
  size_t byte_offset = 0;
  if (napi_get_typedarray_info(env, argv[0], &type, &length, &data,
                               &array_buffer, &byte_offset) != napi_ok ||
      data == nullptr) {
    return false;
  }

  if (type != napi_uint8_array && type != napi_int8_array &&
      type != napi_uint8_clamped_array) {
    napi_throw_type_error(env, nullptr,
                          "compressTemplate expects a Uint8Array");
    return false;
  }

  uint32_t chunk_count = 1;
  if (chunk_count_out != nullptr && argc > 1) {
    napi_valuetype arg1_type = napi_undefined;
    if (napi_typeof(env, argv[1], &arg1_type) == napi_ok &&
        arg1_type == napi_number) {
      napi_get_value_uint32(env, argv[1], &chunk_count);
    }
  }

  *data_out = static_cast<uint8_t*>(data);
  *size_out = length;
  if (chunk_count_out != nullptr) {
    *chunk_count_out = chunk_count;
  }
  return true;
}

napi_value CopyVectorToBuffer(napi_env env, const std::vector<uint8_t>& bytes) {
  napi_value result = nullptr;
  if (napi_create_buffer_copy(env, bytes.size(), bytes.data(), nullptr,
                              &result) != napi_ok) {
    return nullptr;
  }
  return result;
}

napi_value CompressTemplate(napi_env env, napi_callback_info info) {
  uint8_t* input = nullptr;
  size_t length = 0;
  uint32_t chunk_count = 1;
  if (!GetUint8ArrayArgument(env, info, &input, &length, &chunk_count)) {
    return nullptr;
  }

  std::vector<uint8_t> output =
      lynx::tasm::WrapWholeCompressedTemplate(input, length, chunk_count);
  if (output.empty()) {
    return nullptr;
  }
  return CopyVectorToBuffer(env, output);
}

napi_value DecompressTemplate(napi_env env, napi_callback_info info) {
  uint8_t* input = nullptr;
  size_t length = 0;
  if (!GetUint8ArrayArgument(env, info, &input, &length, nullptr)) {
    return nullptr;
  }

  if (length < sizeof(uint32_t) * 2 ||
      !lynx::tasm::IsWholeCompressedTemplate(input, length)) {
    std::vector<uint8_t> passthrough(input, input + length);
    return CopyVectorToBuffer(env, passthrough);
  }

  std::vector<uint8_t> output;
  std::string error_message;
  if (!lynx::tasm::UnwrapWholeCompressedTemplate(input, length, output,
                                                 &error_message)) {
    return nullptr;
  }
  return CopyVectorToBuffer(env, output);
}

}  // namespace

NAPI_MODULE_INIT() {
  napi_property_descriptor properties[] = {
      {"compressTemplate", nullptr, CompressTemplate, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"decompressTemplate", nullptr, DecompressTemplate, nullptr, nullptr,
       nullptr, napi_default, nullptr},
  };
  napi_define_properties(
      env, exports, sizeof(properties) / sizeof(properties[0]), properties);
  return exports;
}
