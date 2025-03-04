// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_BASE_TEMPLATE_READER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_BASE_TEMPLATE_READER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/template_themed.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/runtime/piper/js/js_bundle.h"
#include "core/template_bundle/template_codec/binary_decoder/element_binary_reader.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_config_decoder.h"
#include "core/template_bundle/template_codec/header_ext_info.h"
#include "core/template_bundle/template_codec/moulds.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace tasm {

struct PageRoute;
struct ComponentRoute;

using VersionComponentArray = base::InlineVector<int, 4>;

class LynxBinaryBaseTemplateReader : public ElementBinaryReader {
 private:
  enum class AppType : uint8_t {
    kCard = 0,
    kDynamicComponent,
  };

 public:
  LynxBinaryBaseTemplateReader(std::unique_ptr<lepus::InputStream> stream)
      : ElementBinaryReader(std::move(stream)), support_component_js_(false){};

  ~LynxBinaryBaseTemplateReader() override = default;

  LynxBinaryBaseTemplateReader(const LynxBinaryBaseTemplateReader&) = delete;
  LynxBinaryBaseTemplateReader& operator=(const LynxBinaryBaseTemplateReader&) =
      delete;

  LynxBinaryBaseTemplateReader(LynxBinaryBaseTemplateReader&&) = default;
  LynxBinaryBaseTemplateReader& operator=(LynxBinaryBaseTemplateReader&&) =
      default;

  bool Decode();

  void SetIsCardType(bool is_card) {
    app_type_check_ = is_card ? AppType::kCard : AppType::kDynamicComponent;
  }

  bool IsCardType() { return app_type_check_ == AppType::kCard; }

  std::shared_ptr<ElementTemplateInfo> DecodeElementTemplate(
      const std::string& key);

  size_t GetPageConfigOffset() { return page_config_offset_; }

  void SetTemplateUrl(const std::string& url) { url_ = url; }

 protected:
  // Perform some check or set method after decode header.
  virtual bool DidDecodeHeader() = 0;
  virtual bool DidDecodeAppType();
  virtual bool DidDecodeTemplate() = 0;

  // Decode Header Section.
  bool DecodeHeader();
  bool SupportedLepusVersion(const std::string& binary_version,
                             std::string& error);
  bool CheckLynxVersion(const std::string& binary_version);
  VersionComponentArray VersionStrToNumber(const std::string& version_str);
  bool DecodeHeaderInfo(CompileOptions& compile_options);
  bool DecodeHeaderInfoField();

  template <typename T>
  void ReinterpretValue(T& tgt, const HeaderExtInfoByteArray& src);

  // Decode Template body
  bool DecodeTemplateBody();
  // For Section Route
  bool DecodeSectionRoute();
  // For Specific Section
  bool DecodeSpecificSection(const BinarySection& section);
  // For FlexibleTemplate
  bool DecodeFlexibleTemplateBody();
  // For NonFlexibleTemplate
  bool DeserializeSection();
  // JS section
  bool DeserializeJSSourceSection();
  bool DeserializeJSBytecodeSection();
  // App Descriptor
  bool DecodeAppDescriptor();
  // Page Descriptor
  bool DecodePageDescriptor();
  bool DecodePageRoute(PageRoute& route);
  bool DecodePageMould(PageMould* mould);
  virtual bool DecodeContext() = 0;
  // Lepus Chunk
  virtual bool DecodeLepusChunk() = 0;
  bool DeserializeVirtualNodeSection();
  // Component Descriptor
  bool DecodeComponentDescriptor();
  bool DecodeComponentRoute(ComponentRoute& route);
  bool DecodeComponentMould(ComponentMould* mould, int offset, int length);
  // CSS Descriptor
  virtual bool DecodeCSSDescriptor() = 0;
  // Dynamic Component
  bool DecodeDynamicComponentDescriptor();
  bool DecodeDynamicComponentRoute(DynamicComponentRoute& route);
  bool DecodeDynamicComponentMould(DynamicComponentMould* mould);
  bool DecodeDynamicComponentDeclarations();
  // Themed
  virtual Themed& Themed() = 0;
  bool DecodeThemedSection();

  // Element Template
  virtual bool DecodeElementTemplateSection() = 0;
  // Parsed Style
  virtual bool DecodeParsedStylesSection() = 0;
  virtual ParsedStylesMap& GetParsedStylesMap() = 0;

  // Air Parsed Styles
  virtual bool DecodeAirParsedStylesSection();
  bool DecodeAirParsedStylesInner(StyleMap& style_map);
  virtual AirParsedStylesMap& GetAirParsedStylesMap() = 0;

  // CustomSections
  virtual bool DecodeCustomSectionsSection() = 0;

  // Ensure Page Config
  void EnsurePageConfig();

 protected:
  // If app_type_check_ has value, app type will be checked in DidDecodeAppType,
  // otherwise, this check will be skipped.
  std::optional<AppType> app_type_check_{std::nullopt};
  // config decoder
  std::unique_ptr<LynxBinaryConfigDecoder> config_decoder_;

  // template url
  std::string url_{};

  // header fields.
  uint32_t total_size_;

  bool support_component_js_;
  VersionComponentArray lepus_version_;
  bool is_lepusng_binary_ = false;
  HeaderExtInfo header_ext_info_;
  std::unordered_map<uint32_t, HeaderExtInfoByteArray> header_info_map_;
  lepus::Value template_info_{};
  std::string app_type_{};

  // app fields.
  std::string app_name_{};

  // page fields.
  std::unordered_map<int32_t, std::shared_ptr<PageMould>> page_moulds_{};
  std::shared_ptr<lynx::tasm::PageConfig> page_configs_{};

  // component fields.
  std::unordered_map<std::string, int32_t> component_name_to_id_{};
  std::unordered_map<int32_t, std::shared_ptr<ComponentMould>>
      component_moulds_{};

  // JS fields.
  piper::JsBundle js_bundle_;

  // Dynamic Component fields.
  std::unordered_map<int32_t, std::shared_ptr<DynamicComponentMould>>
      dynamic_component_moulds_;
  // USING_DYNAMIC_COMPONENT_INFO fields.
  std::unordered_map<std::string, std::string>
      dynamic_component_declarations_{};

  // flexible template fields.
  std::unordered_map<BinarySection, TemplateBinary::SectionInfo>
      section_route_{};

  // AirParsedStyles fields
  AirParsedStylesRoute air_parsed_styles_route_;
  // PageConfig chunk offset.
  size_t page_config_offset_ = 0;

  uint64_t decode_start_timestamp_{0};
  uint64_t decode_end_timestamp_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_BASE_TEMPLATE_READER_H_
