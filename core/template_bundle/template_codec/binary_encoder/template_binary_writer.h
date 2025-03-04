// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_TEMPLATE_BINARY_WRITER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_TEMPLATE_BINARY_WRITER_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode_provider_src.h"
#include "core/runtime/vm/lepus/context_binary_writer.h"
#include "core/runtime/vm/lepus/quickjs_debug_info.h"
#include "core/template_bundle/template_codec/binary_encoder/csr_element_binary_writer.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_keyframes_token.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser.h"
#include "core/template_bundle/template_codec/binary_encoder/encode_util.h"
#include "core/template_bundle/template_codec/header_ext_info.h"
#include "core/template_bundle/template_codec/moulds.h"
#include "core/template_bundle/template_codec/template_binary.h"
#include "core/template_bundle/template_codec/ttml_constant.h"

namespace lynx {
namespace tasm {

class CSSParseToken;
class CSSSheet;
struct CSSRoute;
struct ComponentRoute;
struct DynamicComponentRoute;
struct PageRoute;
class PageMould;
class VirtualNode;
class VirtualComponent;

class TemplateAssembler;
class SourceGenerator;

class TemplateBinaryWriter : public CSRElementBinaryWriter {
 public:
  TemplateBinaryWriter(
      lepus::Context* context, bool use_lepusng, bool silence,
      SourceGenerator* parser, CSSParser* css_parser,
      rapidjson::Value* air_styles,
      rapidjson::Value* element_template_parsed_styles,
      rapidjson::Document* element_template, const char* lepus_version,
      const std::string& cli_version, const std::string& app_type,
      const std::string& config, const std::string& lepus_code,
      const std::unordered_map<std::string, std::string>& lepus_chunk_code,
      const CompileOptions& compile_options, const lepus::Value& trial_options,
      const lepus::Value& template_info,
      const std::unordered_map<std::string, std::string> js_code,
      rapidjson::Value* custom_sections, bool enableDebugInfo = false)
      : CSRElementBinaryWriter(context, compile_options, trial_options,
                               enableDebugInfo),
        context_(context),
        use_lepusng_(use_lepusng),
        parser_(parser),
        css_parser_(css_parser),
        air_styles_(air_styles),
        element_template_parsed_styles_(element_template_parsed_styles),
        element_template_(element_template),
        binary_info_(lepus_version, cli_version),
        app_type_(app_type),
        config_(config),
        lepus_code_(lepus_code),
        lepus_chunk_code_(lepus_chunk_code),
        silence_(silence),
        template_info_(template_info),
        js_code_(js_code),
        custom_sections_(custom_sections) {}
  size_t Encode();
  bool WriteToFile(const char* file_name);
  const std::vector<uint8_t> WriteToVector();

  LepusDebugInfo GetDebugInfo() const;
  const std::vector<lynx::fml::RefPtr<lynx::lepus::Function>>& GetContextFunc()
      const;
  const std::map<uint8_t, Range>& OffsetMap() const { return offset_map_; }
  const std::map<BinarySection, uint32_t>& SectionSizeInfo() const {
    return section_size_info_;
  }
  uint32_t HeaderSize() const { return header_size_; }

  const std::unordered_map<
      std::string, std::unique_ptr<piper::quickjs::QuickjsDebugInfoProvider>>&
  GetJsDebugInfo() const {
    return js_debug_info_;
  }
  rapidjson::Value TakeLepusNGDebugInfo() {
    return lepus_debug_info_.TakeDebugInfo();
  }

 protected:
  size_t EncodeNonFlexibleTemplateBody(std::function<void()> encode_func);
  size_t EncodeFlexibleTemplateBody(std::function<void()> encode_func);

  // For flexible template
  void EncodeSectionRoute();
  void MoveLastSectionToFirst(const BinarySection& section);

  // Header Info
  bool EncodeHeaderInfo(const CompileOptions& compile_options);
  bool EncodeHeaderInfoField(
      const HeaderExtInfo::HeaderExtInfoField& header_info_field);

  // CSS Descriptor
  void EncodeCSSDescriptor();
  void EncodeCSSRoute(const CSSRoute& css_route);
  void EncodeCSSFragment(encoder::SharedCSSFragment* fragment);
  bool EncodeCSSParseToken(CSSParseToken* token);
  bool EncodeCSSKeyframesToken(encoder::CSSKeyframesToken* token);
  bool EncodeCSSSheet(CSSSheet* sheet);
  bool EncodeCSSAttributes(const StyleMap& attrs);
  bool EncodeCSSStyleVariables(const CSSVariableMap& style_variables);
  bool EncodeCSSKeyframesMap(const CSSKeyframesMap& keyframes);
  bool EncodeCSSFontFaceToken(CSSFontFaceToken* token);
  bool EncodeCSSFontFaceTokenList(
      const std::vector<std::shared_ptr<CSSFontFaceToken>>& tokenList);
  bool EncodeLynxCSSSelectorTuple(
      const encoder::LynxCSSSelectorTuple& selector_tuple);
  bool EncodeCSSSelector(const css::LynxCSSSelector* selector);

  // JS section
  void SerializeJSSource();
  void EncodeJsBytecode();

  // Encode Header
  void EncodeHeader();
  void EncodeSectionCount(const std::string& app_type);

  // Encode Page config
  void EncodeConfig();

  // Lepus section
  void EncodeLepusSection();
  // Lepus Chunk Section
  void EncodeLepusChunkRoute(const LepusChunkRoute& css_route);
  void EncodeLepusChunkSection();

  // Encode Element Template
  void EncodeElementTemplateSection();
  // Encode ParsedStyle
  void EncodeParsedStylesSection();

  // Encode Air Styles
  void EncodeAirParsedStyles();
  void EncodeAirParsedStylesRoute(const AirParsedStylesRoute& route);

  // encode custom section
  void EncodeCustomSection();
  using CustomSectionHeaders =
      std::vector<std::pair<std::string, CustomSectionHeader>>;
  void EncodeCustomSectionRoute(const CustomSectionHeaders& route);

 private:
  static int FindJSFileInDirectory(
      const char* path, const char* relationPath,
      std::unordered_map<std::string, std::string>& js_map);
  static bool IsDir(const char* path);

 protected:
  lepus::Context* context_;
  bool use_lepusng_;
  SourceGenerator* parser_;
  CSSParser* css_parser_;

  // air styles
  rapidjson::Value* air_styles_{};

  // element template parsed style
  rapidjson::Value* element_template_parsed_styles_{};
  // element template
  rapidjson::Document* element_template_{nullptr};

  TemplateBinary binary_info_;
  std::string app_type_;
  std::string config_;
  std::string lepus_code_;
  std::unordered_map<std::string, std::string> lepus_chunk_code_;
  lynx::lepus::QuickjsDebugInfoBuilder lepus_debug_info_;

  bool silence_;
  PackageInstanceBundleModuleMode bundle_module_mode_;
  HeaderExtInfo header_ext_info_;
  std::map<uint8_t, Range> offset_map_;
  std::map<BinarySection, uint32_t> section_size_info_;
  uint32_t header_size_{0};
  lepus::Value template_info_{};
  std::unordered_map<std::string, std::string> js_code_{};
  std::unordered_map<std::string,
                     std::unique_ptr<piper::quickjs::QuickjsDebugInfoProvider>>
      js_debug_info_{};

  // custom sections
  rapidjson::Value* custom_sections_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_TEMPLATE_BINARY_WRITER_H_
