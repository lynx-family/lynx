// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_ENCODE_UTIL_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_ENCODE_UTIL_H_

#include <memory>
#include <string>
#include <vector>

#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/template_bundle/template_codec/binary_encoder/encoder.h"
#include "core/template_bundle/template_codec/generator/ttml_holder.h"
#include "core/template_bundle/template_codec/template_binary.h"
#include "third_party/rapidjson/document.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

namespace lynx {
namespace lepus {
class Function;
}
namespace tasm {
class SourceGenerator;
class TemplateBinaryWriter;
}  // namespace tasm
}  // namespace lynx

namespace lynx {
namespace tasm {

typedef lynx::fml::RefPtr<lynx::lepus::Function> LepusFunction;

typedef struct LepusNGDebugInfo {
  std::string source_code;
  int32_t end_line_num;
  LEPUSValue top_level_function;
} LepusNGDebugInfo;

struct LepusDebugInfo {
  std::vector<LepusFunction> lepus_funcs_{};
  LepusNGDebugInfo debug_info_{"", -1, LEPUS_UNDEFINED};
};

struct BufferPool {
  std::vector<std::vector<uint8_t>> buffers;
};

void GetScopeInfo(lynx::lepus::Value& scopes, rapidjson::Value& function_scope,
                  rapidjson::Document::AllocatorType& allocator);

void GetLineColInfo(const LepusFunction& current_func,
                    rapidjson::Value& function,
                    rapidjson::Document::AllocatorType& allocator);

void GetChildFunctionInfo(const LepusFunction& current_func,
                          rapidjson::Value& function,
                          rapidjson::Document::AllocatorType& allocator);

void GetDebugInfo(const LepusNGDebugInfo debug_info,
                  rapidjson::Value& template_debug_data,
                  rapidjson::Document::AllocatorType& allocator);

void GetDebugInfo(const std::vector<LepusFunction>& funcs,
                  rapidjson::Value& template_debug_data,
                  rapidjson::Document::AllocatorType& allocator);

void CheckPackageInstance(const PackageInstanceType inst_type,
                          rapidjson::Document& document, std::string& error);

std::string MakeSuccessResult();
std::string MakeErrorResult(const char* errorMessage, const char* file,
                            const char* location);

std::string MakeRepackBufferResult(std::vector<uint8_t>&& data,
                                   BufferPool* pool);

lynx::tasm::EncodeResult CreateSuccessResult(
    const std::vector<uint8_t>& buffer, const std::string& code,
    const std::string& section_size = "", TemplateBinaryWriter* = nullptr);
lynx::tasm::EncodeResult CreateErrorResult(const std::string& error_msg);

lynx::tasm::EncodeResult CreateSSRSuccessResult(
    const std::vector<uint8_t>& buffer);
lynx::tasm::EncodeResult CreateSSRErrorResult(int status,
                                              const std::string& error_msg);

std::string BinarySectionTypeToString(BinarySection section);

bool writefile(const std::string& filename, const std::string& src);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_ENCODE_UTIL_H_
