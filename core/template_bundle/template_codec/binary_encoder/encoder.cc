// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/template_bundle/template_codec/binary_encoder/encoder.h"

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/bindings/lepus/renderer.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser.h"
#include "core/template_bundle/template_codec/binary_encoder/encode_util.h"
#include "core/template_bundle/template_codec/binary_encoder/repack_binary_reader.h"
#include "core/template_bundle/template_codec/binary_encoder/repack_binary_writer.h"
#include "core/template_bundle/template_codec/binary_encoder/template_binary_writer.h"
#include "core/template_bundle/template_codec/generator/base_struct.h"
#include "core/template_bundle/template_codec/generator/meta_factory.h"
#include "core/template_bundle/template_codec/generator/source_generator.h"
#include "core/template_bundle/template_codec/generator/template_parser.h"
#include "core/template_bundle/template_codec/generator/template_scope.h"
#include "core/template_bundle/template_codec/ttml_constant.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"
#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

#include "third_party/aes/aes.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif
#include "core/runtime/vm/lepus/bytecode_generator.h"
#ifdef OS_IOS
#include "trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace tasm {

namespace {
#define IF_FAIL_RETURN                                  \
  if (!encoder_options.parser_result_) {                \
    return CreateErrorResult(encoder_options.err_msg_); \
  }

std::string GetExceptionMessage(LEPUSContext* ctx) {
  LEPUSValue exception_val = LEPUS_GetException(ctx);
  HandleScope func_scope(ctx, &exception_val, HANDLE_TYPE_LEPUS_VALUE);
  LEPUSValue val;
  const char* stack;
  const char* message = LEPUS_ToCString(ctx, exception_val);
  std::string ret = "quickjs: ";
  if (message) {
    ret += message;
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, message);
  }

  bool is_error = LEPUS_IsError(ctx, exception_val);
  if (is_error) {
    val = LEPUS_GetPropertyStr(ctx, exception_val, "stack");
    func_scope.PushHandle(&val, HANDLE_TYPE_LEPUS_VALUE);
    if (!LEPUS_IsUndefined(val)) {
      stack = LEPUS_ToCString(ctx, val);
      ret += stack;
      if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeCString(ctx, stack);
    }
    if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, val);
  }
  if (!LEPUS_IsGCMode(ctx)) LEPUS_FreeValue(ctx, exception_val);
  return ret;
}
}  // namespace

std::unique_ptr<SourceGenerator> GetTTMLParser(
    const EncoderOptions& encoder_options) {
  return SourceGenerator::GenerateParser(encoder_options);
}

std::unique_ptr<SourceGenerator> ParserTTML(CSSParser* css_parser,
                                            EncoderOptions& encoder_options) {
  if (!encoder_options.generator_options_.silence_) {
    printf("    parsing ttml...\n");
  }

  if (encoder_options.source_generator_options_.enable_tt_for_full_version) {
    std::cout << "use enableTTForFullVersion..." << std::endl;
  }

  std::unique_ptr<lynx::tasm::SourceGenerator> ttml_parser =
      GetTTMLParser(encoder_options);
  if (!ttml_parser) {
    encoder_options.parser_result_ = false;
    encoder_options.err_msg_ =
        "No Parser Implementation for Non-Card and Non-DynamicComponent";
    return ttml_parser;
  }
  ttml_parser->SetClosureFix(
      encoder_options.generator_options_.lepus_closure_fix_);

  // if enable fiber arch, no need to parse ttml
  if (encoder_options.compile_options_.enable_fiber_arch_) {
    return ttml_parser;
  }

  CheckPackageInstance(
      encoder_options.generator_options_.app_type_ == APP_TYPE_CARD
          ? PackageInstanceType::CARD
          : PackageInstanceType::DYNAMIC_COMPONENT,
      encoder_options.generator_options_.source_content_obj_,
      encoder_options.err_msg_);

  // Prepare ttss ids
  std::unordered_map<std::string, uint32_t> ttss_ids;
  for (auto it = css_parser->fragments().begin();
       it != css_parser->fragments().end(); ++it) {
    ttss_ids.insert({it->first, it->second->id()});
  }
  ttml_parser->set_ttss_ids(ttss_ids);
  try {
    ttml_parser->Parse();
  } catch (lynx::lepus::ParseException& e) {
    std::string err_str =
        MakeErrorResult(e.msg_.c_str(), e.file_.c_str(), e.location_.c_str());
    encoder_options.parser_result_ = false;
    encoder_options.err_msg_ = err_str;
    return ttml_parser;
  } catch (lynx::lepus::CompileException& e) {
    std::string err_str = MakeErrorResult(e.message().c_str(), "", "");
    encoder_options.parser_result_ = false;
    encoder_options.err_msg_ = err_str;
    return ttml_parser;
  }
  return ttml_parser;
}

std::shared_ptr<lynx::lepus::Context> GetVMContent(
    const EncoderOptions& encoder_options) {
  std::shared_ptr<lynx::lepus::Context> vm_context(
      lynx::lepus::Context::CreateContext(
          encoder_options.compile_options_.enable_lepus_ng_));

  if (vm_context && vm_context->IsVMContext()) {
    lynx::lepus::VMContext::Cast(vm_context.get())
        ->SetSdkVersion(encoder_options.compile_options_.target_sdk_version_);
    lynx::lepus::VMContext::Cast(vm_context.get())
        ->SetClosureFix(encoder_options.generator_options_.lepus_closure_fix_);
  }
  vm_context->Initialize();

  // When encoder, $kTemplateAssembler only needs a pointer address.
  // It's address is not used, so we can provide any address.
  void* assembler = vm_context.get();
  if (vm_context->IsLepusNGContext()) {
    lynx::lepus::QuickContext* quick_ctx =
        lynx::lepus::QuickContext::Cast(vm_context.get());

    if (encoder_options.compile_options_.lepusng_debuginfo_outside_) {
      quick_ctx->set_debuginfo_outside(true);
      SetDebugInfoOutside(quick_ctx->context(), true);
    }
    LEPUSValue self =
        LEPUS_MKPTR(LEPUS_TAG_LEPUS_CPOINTER,
                    static_cast<lepus::Context::Delegate*>(assembler));
    quick_ctx->RegisterGlobalProperty("$kTemplateAssembler", self);
    Utils::RegisterNGBuiltin(vm_context.get());
    Renderer::RegisterNGBuiltin(vm_context.get(),
                                encoder_options.compile_options_.arch_option_);
  } else {
    lynx::lepus::Value self(static_cast<lepus::Context::Delegate*>(assembler));
    lynx::lepus::VMContext::Cast(vm_context.get())
        ->SetGlobalData("$kTemplateAssembler", self);
    Utils::RegisterBuiltin(vm_context.get());
    Renderer::RegisterBuiltin(vm_context.get(),
                              encoder_options.compile_options_.arch_option_);
  }
  return vm_context;
}

std::unique_ptr<CSSParser> ParserCSS(EncoderOptions& encoder_options) {
  if (!encoder_options.generator_options_.silence_) {
    printf("    parsing ttss...\n");
  }

  auto css_parser =
      std::make_unique<CSSParser>(encoder_options.compile_options_);
  try {
    if (encoder_options.compile_options_.enable_fiber_arch_) {
      css_parser->ParseCSSForFiber(
          encoder_options.generator_options_.css_map_,
          encoder_options.generator_options_.css_source_);
    } else {
      css_parser->Parse(encoder_options.generator_options_.css_obj_);
    }
  } catch (lynx::lepus::ParseException& e) {
    std::string err_str =
        MakeErrorResult(e.msg_.c_str(), e.file_.c_str(), e.location_.c_str());
    encoder_options.parser_result_ = false;
    encoder_options.err_msg_ = err_str;
    return css_parser;
  } catch (lynx::lepus::CompileException& e) {
    std::string err_str = MakeErrorResult(e.message().c_str(), "", "");
    encoder_options.parser_result_ = false;
    encoder_options.err_msg_ = err_str;
    return css_parser;
  }
  return css_parser;
}

std::unique_ptr<TemplateBinaryWriter> EncodeTemplate(
    CSSParser* css_parser, SourceGenerator* ttml_parser,
    lepus::Context* vm_context, EncoderOptions& encoder_options) {
  if (!encoder_options.generator_options_.silence_) {
    printf("    encoding...\n");
    printf("engine version:%s\n", targetSdkVersion);
    printf("lepus version:%s\n", lepusVersion);
  }

  auto encoder = std::make_unique<TemplateBinaryWriter>(
      vm_context, encoder_options.compile_options_.enable_lepus_ng_,
      encoder_options.generator_options_.silence_, ttml_parser, css_parser,
      &encoder_options.generator_options_.air_parsed_styles_,
      &encoder_options.generator_options_.parsed_styles_,
      &encoder_options.generator_options_.element_template_,
      encoder_options.generator_options_.lepus_version_.c_str(),
      encoder_options.generator_options_.cli_version_,
      encoder_options.generator_options_.app_type_,
      encoder_options.generator_options_.config_,
      encoder_options.generator_options_.lepus_code_,
      encoder_options.generator_options_.lepus_chunk_code_,
      encoder_options.compile_options_,
      encoder_options.generator_options_.trial_options_,
      encoder_options.generator_options_.template_info_,
      encoder_options.generator_options_.js_code_,
      &encoder_options.generator_options_.custom_sections_,
      encoder_options.generator_options_.enable_debug_info_);
  try {
    size_t binary_size = encoder->Encode();
    if (binary_size == 0) {
      std::stringstream ss;
      ss << "error: encode failed:";
      encoder_options.parser_result_ = false;
      encoder_options.err_msg_ = ss.str();
      return encoder;
    }
  } catch (lynx::lepus::CompileException& e) {
    std::string err_str = MakeErrorResult(e.message().c_str(), "", "");
    encoder_options.parser_result_ = false;
    encoder_options.err_msg_ = err_str;
    return encoder;
  }
  return encoder;
}

std::pair<std::vector<uint8_t>, std::string> GenerateEncodeResult(
    TemplateBinaryWriter* encoder, EncoderOptions& encoder_options) {
  auto encode_result = encoder->WriteToVector();
  uint32_t header_size = encoder->HeaderSize();
  auto& section_size_info = encoder->SectionSizeInfo();
  auto section_document = rapidjson::Document{};
  section_document.SetObject();
  auto& allocator = section_document.GetAllocator();
  section_document.AddMember("header", header_size, allocator);
  for (const auto& pair : section_size_info) {
    section_document.AddMember(
        rapidjson::Value(BinarySectionTypeToString(pair.first).c_str(),
                         allocator),
        rapidjson::Value(pair.second), allocator);
  }
  rapidjson::StringBuffer buf;
  rapidjson::Writer<rapidjson::StringBuffer> doc_writer(buf);
  section_document.Accept(doc_writer);
  std::string section_size = buf.GetString();

  // encode vector to buffer
  std::uint32_t size = encode_result.size() + sizeof(std::uint32_t);
  if (encoder_options.generator_options_.enable_ssr_ ||
      encoder_options.generator_options_.enable_cursor_) {
    std::vector<uint8_t> suffix;
    suffix.push_back(static_cast<uint8_t>(encoder->OffsetMap().size()));
    for (const auto& pair : encoder->OffsetMap()) {
      Range range = pair.second;
      range.start += sizeof(uint32_t);
      range.end += sizeof(uint32_t);
      suffix.push_back(pair.first);
      std::vector<uint8_t> start_vec(
          (uint8_t*)(&range.start),
          (uint8_t*)(&range.start) + sizeof(uint32_t));
      std::vector<uint8_t> end_vec((uint8_t*)(&range.end),
                                   (uint8_t*)(&range.end) + sizeof(uint32_t));
      suffix.insert(suffix.end(), start_vec.begin(), start_vec.end());
      suffix.insert(suffix.end(), end_vec.begin(), end_vec.end());
    }

    uint32_t magic = template_codec::kTasmSsrSuffixMagic;
    std::vector<uint8_t> magic_vec((uint8_t*)(&magic),
                                   (uint8_t*)(&magic) + sizeof(uint32_t));
    uint32_t suffix_size = suffix.size() + 2 * sizeof(uint32_t);
    std::vector<uint32_t> suffix_size_vec(
        (uint8_t*)(&suffix_size), (uint8_t*)(&suffix_size) + sizeof(uint32_t));
    suffix.insert(suffix.end(), suffix_size_vec.begin(), suffix_size_vec.end());
    suffix.insert(suffix.end(), magic_vec.begin(), magic_vec.end());

    size += suffix.size();
    encode_result.insert(encode_result.end(), suffix.begin(), suffix.end());
  }
  std::vector<std::uint8_t> prefix(
      (std::uint8_t*)&size, (std::uint8_t*)&(size) + sizeof(std::uint32_t));
  std::string sample = std::string(prefix.begin(), prefix.end()) +
                       std::string(encode_result.begin(), encode_result.end());
  auto str = std::make_unique<std::string>(sample);
  auto u_char_arr = reinterpret_cast<unsigned const char*>(str->c_str());
  auto length = sample.size();
  std::vector<uint8_t> buffer(u_char_arr, u_char_arr + length);

  if (!encoder_options.generator_options_.silence_) {
    printf("Binary Encode success! \n");
  }
  return std::make_pair(buffer, section_size);
}

lynx::tasm::EncodeResult EncodeInner(const std::string& options_str) {
  // To ensure the stability of the compiled output in concurrent scenarios,
  // reset before each compilation. TODO: Address instability in multithreading
  // scenarios
  sComponentInstanceIdGenerator = 0;

  // Parse input str to json object
  rapidjson::Document options;
  if (options.Parse(options_str).HasParseError()) {
    std::stringstream ss;
    ss << "Parse '" << options_str << "' error. Source is not valid json file. "
       << options.GetParseErrorMsg()
       << " Error position: " << options.GetErrorOffset();
    return CreateErrorResult(ss.str());
  }

  // step 0: get encode options
  EncoderOptions encoder_options = MetaFactory::GetEncoderOptions(options);
  IF_FAIL_RETURN

  // step 1: parser ttss
  auto css_parser = ParserCSS(encoder_options);
  IF_FAIL_RETURN

  // step 2: compile ttml
  auto ttml_parser = ParserTTML(css_parser.get(), encoder_options);
  IF_FAIL_RETURN

  if (encoder_options.generator_options_.skip_encode_) {
    // If skip encode, return.
    std::vector<uint8_t> buffer;
    return CreateSuccessResult(buffer, ttml_parser->GetLepusCode());
  }

  // step 3: init vm context
  auto vm_context = GetVMContent(encoder_options);

  // step 4: encode template
  auto encoder = EncodeTemplate(css_parser.get(), ttml_parser.get(),
                                vm_context.get(), encoder_options);
  IF_FAIL_RETURN

  // step 5: generate template
  auto res = GenerateEncodeResult(encoder.get(), encoder_options);

  // step 6: return success result
  return CreateSuccessResult(std::move(res.first), ttml_parser->GetLepusCode(),
                             res.second, encoder.get());
}

lynx::tasm::EncodeResult encode_ssr_inner(const uint8_t* ptr, size_t buf_len,
                                          const std::string& mixin_data) {
  rapidjson::Document mix_doc;
  if (mix_doc.Parse(mixin_data).HasParseError()) {
    std::stringstream ss;
    ss << "error: mixin data is not valid json format! msg: "
       << mix_doc.GetParseErrorMsg()
       << ", position: " << mix_doc.GetErrorOffset();
    return CreateSSRErrorResult(EncodeSSRError::ERR_MIX_DATA, ss.str());
  }

  auto input_stream =
      std::make_unique<lynx::lepus::ByteArrayInputStream>(ptr, buf_len);
  std::shared_ptr<lynx::lepus::Context> vm_context(
      lynx::lepus::Context::CreateContext());
  RepackBinaryReader reader(vm_context.get(), std::move(input_stream));

  if (!reader.DecodeHeader() || !reader.DecodeSuffix() ||
      !reader.DecodeString()) {
    return CreateSSRErrorResult(reader.error_code(), reader.error_message_);
  }

  lepus_value page_value;
  for (auto& m : mix_doc.GetObject()) {
    if (std::string(m.name.GetString()).compare("data") == 0) {
      page_value = lynx::lepus::jsonValueTolepusValue(m.value);
    }
  }
  if (page_value.IsNil()) {
    return CreateSSRErrorResult(EncodeSSRError::ERR_DATA_EMPTY,
                                "Mix Data Empty");
  }

  RepackBinaryWriter writer(reader.context(), reader.compile_options());
  writer.EncodeValue(&page_value);
  writer.EncodeString();

  std::vector<uint8_t> new_data_vec = writer.GetDataBuffer();
  std::map<uint8_t, Range> offset_map = reader.offset_map();
  uint32_t data_gap = 0;
  Range data_range =
      reader.is_card()
          ? offset_map[BinaryOffsetType::TYPE_PAGE_DATA]
          : offset_map[BinaryOffsetType::TYPE_DYNAMIC_COMPONENT_DATA];
  data_gap = new_data_vec.size() - data_range.size();

  if (reader.is_card()) {
    PageRoute route;
    if (!reader.DecodePageRoute(route)) {
      return CreateSSRErrorResult(reader.error_code(), reader.error_message_);
    }
    route.page_ranges[0].end += data_gap;
    writer.EncodePageRoute(route);
  } else {
    DynamicComponentRoute route;
    if (!reader.DecodeDynamicComponentRoute(route)) {
      return CreateSSRErrorResult(reader.error_code(), reader.error_message_);
    }
    route.dynamic_component_ranges[0].end += data_gap;
    writer.EncodeDynamicComponentRoute(route);
  }

  std::vector<uint8_t> new_template;
  writer.AssembleNewTemplate(ptr, buf_len, reader.suffix_size(),
                             reader.string_offset(), offset_map,
                             reader.is_card(), new_template);
  uint32_t total_size = new_template.size();
  std::vector<uint8_t> prefix(
      reinterpret_cast<const uint8_t*>(&total_size),
      reinterpret_cast<const uint8_t*>(&total_size) + sizeof(total_size));
  for (auto i = 0; i < prefix.size(); ++i) {
    new_template[i] = prefix[i];
  }

  return CreateSSRSuccessResult(new_template);
}

const char* reencode_template_debug_inner(const uint8_t* ptr, uint32_t len,
                                          const char* template_debug_url,
                                          BufferPool* pool) {
  auto input_stream =
      std::make_unique<lynx::lepus::ByteArrayInputStream>(ptr, len);
  std::shared_ptr<lynx::lepus::Context> vm_context(
      lynx::lepus::Context::CreateContext());
  RepackBinaryReader reader(vm_context.get(), std::move(input_stream));

  if (!reader.DecodeHeader()) {
    return strdup(reader.error_message_.c_str());
  }

  std::string res_str;
  CompileOptions compile_options = reader.compile_options();
  if (!Config::IsHigherOrEqual(compile_options.target_sdk_version_,
                               FEATURE_HEADER_EXT_INFO_VERSION)) {
    res_str = MakeErrorResult(
        "template file sdk version error, should be 1.6 or higher", "", "");
    return strdup(res_str.c_str());
  }

  if (!reader.DecodeHeaderInfo()) {
    return strdup(reader.error_message_.c_str());
  }

  compile_options.template_debug_url_ = std::string(template_debug_url);

  RepackBinaryWriter writer(reader.context(), compile_options);
  writer.EncodeHeaderInfo(compile_options);

  std::vector<uint8_t> new_template;
  writer.AssembleTemplateWithNewHeaderInfo(
      ptr, len, reader.header_ext_info_offset(), reader.header_ext_info_size(),
      new_template);
  uint32_t total_size = new_template.size();
  std::vector<uint8_t> prefix(
      reinterpret_cast<const uint8_t*>(&total_size),
      reinterpret_cast<const uint8_t*>(&total_size) + sizeof(total_size));
  for (auto i = 0; i < prefix.size(); ++i) {
    new_template[i] = prefix[i];
  }

  res_str = MakeRepackBufferResult(std::move(new_template), pool);
  return strdup(res_str.c_str());
}

std::string quickjsCheck(const std::string& sourceFileOrigin) {
  const char* sourceFile = sourceFileOrigin.c_str();
  LEPUSRuntime* rt;
  rt = LEPUS_NewRuntime();
  LEPUSContext* ctx = LEPUS_NewContext(rt);
  int eval_flags;
  LEPUSValue obj;

  eval_flags = LEPUS_EVAL_FLAG_COMPILE_ONLY | LEPUS_EVAL_TYPE_GLOBAL;
  obj = LEPUS_Eval(ctx, sourceFile, strlen(sourceFile), "", eval_flags);

  if (LEPUS_IsException(obj)) {
    std::string err_str =
        MakeErrorResult(GetExceptionMessage(ctx).c_str(), "", "");
    LEPUS_FreeContext(ctx);
    LEPUS_FreeRuntime(rt);
    return std::string(err_str.c_str());
  }

  std::string res_str = MakeSuccessResult().c_str();

  LEPUS_FreeContext(ctx);
  LEPUS_FreeRuntime(rt);
  return std::string(res_str.c_str());
}

lynx::tasm::EncodeResult encode(const std::string& options_str) {
  try {
    return EncodeInner(options_str);
  } catch (const lynx::lepus::Exception& e) {
    std::string err_str = MakeErrorResult(e.message().c_str(), "", "");
    return CreateErrorResult(err_str.c_str());
  }
}

lynx::tasm::EncodeResult encode_ssr(const uint8_t* ptr, size_t buf_len,
                                    const std::string& mixin_data) {
  return encode_ssr_inner(ptr, buf_len, mixin_data);
}

}  // namespace tasm
}  // namespace lynx

extern "C" {
lynx::tasm::BufferPool* createBufferPool() {
  return new lynx::tasm::BufferPool();
}

void dropBufferPool(lynx::tasm::BufferPool* pool) { delete pool; }

const uint8_t* readBufferPool_data(const lynx::tasm::BufferPool* pool,
                                   unsigned int index) {
  if (index < pool->buffers.size())
    return &pool->buffers[index][0];
  else
    return nullptr;
}

size_t readBufferPool_len(const lynx::tasm::BufferPool* pool,
                          unsigned int index) {
  if (index < pool->buffers.size())
    return pool->buffers[index].size();
  else
    return 0;
}

const char* reencode_template_debug(const uint8_t* ptr, uint32_t len,
                                    const char* template_debug_url,
                                    lynx::tasm::BufferPool* pool) {
  if (ptr == nullptr || len == 0) {
    std::string res_str =
        lynx::tasm::MakeErrorResult("template file is null!", "", "");
    return strdup(res_str.c_str());
  }
  if (template_debug_url == nullptr) {
    std::string res_str =
        lynx::tasm::MakeErrorResult("template debug url is empty!", "", "");
    return strdup(res_str.c_str());
  }
  return lynx::tasm::reencode_template_debug_inner(ptr, len, template_debug_url,
                                                   pool);
}

// execute flag:
// 1: do lepusCheck && lepus Execute
// 2: run quickjsCheck
// else: do lepusCheck
const char* lepusCheck(const char* sourceFile, char* targetSdkVersion,
                       int execute) {
  if (execute == 2) {
    return strdup(lynx::tasm::quickjsCheck(std::string(sourceFile)).c_str());
  }
  lynx::lepus::VMContext context;
  context.Initialize();
  lynx::lepus::Value obj(lynx::lepus::Dictionary::Create());
  context.SetGlobalData("exports", obj);
  lynx::tasm::Utils::RegisterBuiltin(&context);
  // TODO(songshourui.null): when exec lepusCheck, pass the arch option. If miss
  // the option, use RADON_ARCH as default.
  lynx::tasm::Renderer::RegisterBuiltin(&context,
                                        lynx::tasm::ArchOption::RADON_ARCH);
  std::string err_str = lynx::lepus::BytecodeGenerator::GenerateBytecode(
      &context, sourceFile, targetSdkVersion);
  if (err_str.length() > 0) {
    return strdup(err_str.c_str());
  }
  if (execute == 1) {
    context.Execute();
  }

  std::string res_str = lynx::tasm::MakeSuccessResult().c_str();
  return strdup(res_str.c_str());
}
}
#ifdef __EMSCRIPTEN__
extern "C" {
void leakCheck(const char* msg) {
  // Uncomment when analyzing memory leaks
  //__lsan_do_leak_check();
}
}
#endif

// wasm binding

#if defined(__EMSCRIPTEN__)

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
#endif
