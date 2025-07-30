// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"

#include <algorithm>
#include <string>
#include <vector>

#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/template_bundle/template_codec/binary_decoder/binary_decoder_trace_event_def.h"
#include "core/template_bundle/template_codec/template_binary.h"
#include "lynx_binary_base_css_reader.h"

namespace lynx {
namespace tasm {

bool LynxBinaryReader::DidDecodeHeader() {
  // construct config decoder
  config_decoder_ = std::make_unique<LynxBinaryConfigDecoder>(
      compile_options_, compile_options_.target_sdk_version_,
      is_lepusng_binary_, enable_css_parser_);

  if (template_info_.IsObject()) {
    EnsurePageConfig();

    // Determine whether to enable ParallelElement through
    // kEnableParallelElement and kEnableConcurrentElement. At present, some
    // online templates are using the kEnableConcurrentElement switch. After the
    // subsequent templates is taken offline, kEnableConcurrentElement will be
    // deleted.
    BASE_STATIC_STRING_DECL(kEnableConcurrentElement,
                            "enableConcurrentElement");
    page_configs_->SetEnableParallelElement(
        template_info_.GetProperty(BASE_STATIC_STRING(kEnableParallelElement))
            .Bool() ||
        template_info_.GetProperty(kEnableConcurrentElement).Bool());
  }

  auto& tb = template_bundle();
  tb.total_size_ = total_size_;
  tb.is_lepusng_binary_ = is_lepusng_binary_;
  tb.target_sdk_version_ = compile_options_.target_sdk_version_;
  tb.compile_options_ = compile_options_;
  tb.template_info_ = template_info_;
  tb.enable_css_parser_ = enable_css_parser_;
  tb.enable_css_variable_ = enable_css_variable_;
  tb.support_component_js_ = support_component_js_;
  return true;
}

bool LynxBinaryReader::DidDecodeAppType() {
  template_bundle().app_type_ = app_type_;
  return LynxBinaryBaseTemplateReader::DidDecodeAppType();
}

bool LynxBinaryReader::DidDecodeTemplate() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, BINARY_READER_DID_DECODE_TEMPLATE);

  if (IsCardType() && !page_configs_) {
    error_message_ = "DecodeAppError: page config is null";
    return false;
  }

  auto& tb = template_bundle();
  tb.app_name_ = app_name_;
  tb.page_moulds_ = std::move(page_moulds_);
  tb.component_moulds_ = std::move(component_moulds_);
  tb.component_name_to_id_ = std::move(component_name_to_id_);
  tb.js_bundle_ = std::move(js_bundle_);
  tb.page_configs_ = std::move(page_configs_);
  tb.dynamic_component_moulds_ = std::move(dynamic_component_moulds_);
  tb.dynamic_component_declarations_ =
      std::move(dynamic_component_declarations_);
  return true;
}

Themed& LynxBinaryReader::Themed() { return template_bundle().themed_; }

bool LynxBinaryReader::DecodeCSSDescriptor() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, BINARY_READER_DECODE_CSS_DESCRIPTOR);
  // decode route
  ERROR_UNLESS(DecodeCSSDescriptorRoute());

  // greedy decode css section
  const auto& manager = template_bundle().GetCSSStyleManager();

  if (page_configs_) {
    manager->SetFixCSSImportRuleOrder(
        page_configs_->GetFixCSSImportRuleOrder());
  }

  ERROR_UNLESS(GreedyDecodeCSSDescriptor((*manager->GetCSSFragmentMap())));

  // make all fragments read-only
  manager->FlattenAllCSSFragment();

  return true;
}

bool LynxBinaryReader::DecodeStyleObjects() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeStyleObjectSection");
  DECODE_COMPACT_U32(section_count);
  // Decode style_object section
  if (section_count <
      static_cast<uint32_t>(StyleObjectSectionType::STYLE_OBJECT)) {
    return true;
  }
  StyleObjectRoute route;
  ERROR_UNLESS(DecodeStyleObjectRoute(route));
  const auto& style_object_list =
      template_bundle().InitStyleObjectList(route.style_object_ranges.size());
  size_t index = 0;
  for (const auto& range : route.style_object_ranges) {
    stream_->Seek(style_objects_section_range_.start + range.start);
    DECODE_COMPACT_U32(size);
    StyleMap attr;
    ERROR_UNLESS(DecodeCSSAttributes(attr, size));
    (style_object_list.get())[index++] = new style::StyleObject(attr);
    stream_->Seek(style_objects_section_range_.start + range.end);
  }
  // Decode keyframes section
  if (section_count <
      static_cast<uint32_t>(StyleObjectSectionType::STYLE_OBJECT_KEYFRAMES)) {
    return true;
  }
  StyleObjectRoute keyframes_route;
  ERROR_UNLESS(DecodeStyleObjectRoute(keyframes_route));
  auto parser_config =
      CSSParserConfigs::GetCSSParserConfigsByComplierOptions(compile_options_);
  auto keyframes = template_bundle().InitKeyframesMap(
      keyframes_route.style_object_ranges.size());
  for (const auto& range : keyframes_route.style_object_ranges) {
    stream_->Seek(style_objects_section_range_.start + range.start);
    DECODE_STDSTR(name);
    CSSKeyframesToken* token = new CSSKeyframesToken(parser_config);
    ERROR_UNLESS(DecodeCSSKeyframesToken(token));
    keyframes.emplace(std::move(name), token);
    stream_->Seek(style_objects_section_range_.start + range.end);
  }
  return true;
}

bool LynxBinaryReader::DecodeCSSDescriptorRoute() {
  const auto& manager = template_bundle().GetCSSStyleManager();
  manager->SetEnableNewImportRule(GetEnableNewImportRule());
  return DecodeCSSRoute(manager->route_);
}

bool LynxBinaryReader::GreedyDecodeCSSDescriptor(
    CSSStyleSheetManager::CSSFragmentMap& css_fragment_map) {
  auto& fragment_ranges =
      template_bundle().GetCSSStyleManager()->route_.fragment_ranges;
  const auto& manager = template_bundle().GetCSSStyleManager();
  for (auto it = fragment_ranges.begin(); it != fragment_ranges.end(); ++it) {
    auto fragment = std::make_unique<SharedCSSFragment>(manager.get());
    stream_->Seek(css_section_range_.start + it->second.start);
    ERROR_UNLESS(DecodeCSSFragment(fragment.get(),
                                   it->second.end + css_section_range_.start));
    fragment->SetEnableClassMerge(compile_options_.enable_css_class_merge_);
    auto fragment_id = fragment->id();
    css_fragment_map.emplace(fragment_id, std::move(fragment));
  }
  stream_->Seek(css_section_range_.end);
  return true;
}

bool LynxBinaryReader::DecodeLepusChunk() {
  // TODO(luochangan.adrian): Evaluation of the necessity of lazy decoding
  TRACE_EVENT(LYNX_TRACE_CATEGORY, BINARY_READER_DECODE_LEPUS_CHUNK);
  ERROR_UNLESS(DecodeLepusChunkRoute());
  ERROR_UNLESS(GreedyDecodeLepusChunk(
      template_bundle().lepus_chunk_manager_->lepus_chunk_map_));

  return true;
}

bool LynxBinaryReader::GreedyDecodeLepusChunk(
    LepusChunkManager::LepusChunkMap& chunk_map) {
  const auto& start_offsets = lepus_chunk_route_.start_offsets_;
  for (auto it = start_offsets.begin(); it != start_offsets.end(); ++it) {
    stream_->Seek(lepus_chunk_route_.descriptor_offset_ + it->second);

    chunk_map[it->first] = lepus::ContextBundle::Create(is_lepusng_binary_);

    ERROR_UNLESS(chunk_map[it->first]);
    ERROR_UNLESS(DecodeContextBundle(chunk_map[it->first].get()));
  }
  return true;
}

bool LynxBinaryReader::DecodeLepusChunkRoute() {
  DECODE_COMPACT_U32(size);
  uint32_t lepus_chunk_length = 0;
  for (size_t i = 0; i < size; ++i) {
    std::string path;
    ERROR_UNLESS(ReadStringDirectly(&path));
    // LepusChunkRange
    DECODE_COMPACT_U32(start);
    DECODE_COMPACT_U32(end);
    lepus_chunk_route_.start_offsets_.emplace(
        std::piecewise_construct, std::forward_as_tuple(std::move(path)),
        std::forward_as_tuple(start));
    lepus_chunk_length = std::max(end, lepus_chunk_length);
  }
  lepus_chunk_route_.descriptor_offset_ =
      static_cast<uint32_t>(stream_->offset());
  lepus_chunk_range_.start = lepus_chunk_route_.descriptor_offset_;
  lepus_chunk_range_.end = lepus_chunk_range_.start + lepus_chunk_length;
  return true;
}

bool LynxBinaryReader::DecodeContext() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, BINARY_READER_DECODE_CONTEXT);
  auto& tb = template_bundle();
  tb.context_bundle_ = lepus::ContextBundle::Create(is_lepusng_binary_);

  ERROR_UNLESS(tb.context_bundle_);
  ERROR_UNLESS(DecodeContextBundle(tb.context_bundle_.get()));
  return true;
}

bool LynxBinaryReader::DecodeParsedStylesSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, BINARY_READER_DECODE_PARSED_STYLES_SECTION);
  ERROR_UNLESS(DecodeParsedStylesRouter());
  ERROR_UNLESS(GreedyDecodeParsedStylesSection());
  return true;
}

bool LynxBinaryReader::GreedyDecodeParsedStylesSection() {
  for (const auto& [key, offset] :
       string_key_parsed_styles_router_.start_offsets_) {
    GetParsedStylesMap().emplace(key, GetParsedStyles(key));
  }
  return true;
}

bool LynxBinaryReader::DecodeElementTemplateSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              BINARY_READER_DECODE_ELEMENT_TEMPLATE_SECTION);
  ERROR_UNLESS(DecodeElementTemplatesRouter());
  ERROR_UNLESS(GreedyDecodeElementTemplateSection());
  ERROR_UNLESS(GreedyConstructElements());
  return true;
}

bool LynxBinaryReader::DecodeCustomSectionsSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, BINARY_READER_DECODE_CUSTOM_SECTIONS);
  DECODE_U32(size);

  CustomSectionRoute route;
  for (uint32_t i = 0; i < size; ++i) {
    std::string key{};
    lepus::Value header{};
    ERROR_UNLESS(ReadStringDirectly(&key));
    ERROR_UNLESS(DecodeValue(&header, false));
    DECODE_U32(start);
    DECODE_U32(end);
    route.custom_section_headers.emplace(
        std::move(key),
        CustomSectionHeader{std::move(header), Range{start, end}});
  }
  route.descriptor_offset = static_cast<uint32_t>(stream_->offset());

  ERROR_UNLESS(DecodeCustomSectionsByRoute(route));

  return true;
}

bool LynxBinaryReader::DecodeCustomSectionsByRoute(
    const CustomSectionRoute& route) {
  auto& tb = template_bundle();
  for (const auto& [key, header] : route.custom_section_headers) {
    stream_->Seek(route.descriptor_offset + header.range.start);
    lepus::Value content{};
    CustomSectionEncodingType encoding_type = CustomSectionEncodingType::STRING;
    if (header.header.IsTable()) {
      BASE_STATIC_STRING_DECL(kEncodingType, "encoding");
      auto maybe_encoding_type = header.header.GetProperty(kEncodingType);
      if (maybe_encoding_type.IsNumber()) {
        auto type_num = maybe_encoding_type.Number();
        if (type_num ==
            static_cast<int>(CustomSectionEncodingType::JS_BYTECODE)) {
          encoding_type = CustomSectionEncodingType::JS_BYTECODE;
        }
      }
    }
    switch (encoding_type) {
      case CustomSectionEncodingType::STRING:
        ERROR_UNLESS(DecodeValue(&content, false));
        break;
      case CustomSectionEncodingType::JS_BYTECODE:
        ERROR_UNLESS(is_lepusng_binary_);
        uint64_t code_len;
        ERROR_UNLESS(ReadCompactU64(&code_len));
        auto data = std::make_unique<uint8_t[]>(code_len);
        ERROR_UNLESS(ReadData(data.get(), static_cast<int>(code_len)));
        content.SetByteArray(
            lepus::ByteArray::Create(std::move(data), code_len));
        break;
    }
    tb.AddCustomSection(key, content);
  }
  return true;
}

bool LynxBinaryReader::GreedyDecodeElementTemplateSection() {
  auto& tb = template_bundle();
  for (const auto& [key, offset] : element_templates_router_.start_offsets_) {
    auto info = DecodeTemplatesInfoWithKey(key);
    tb.element_template_infos_.emplace(key, std::move(info));
  }
  return true;
}

bool LynxBinaryReader::GreedyConstructElements() {
  // TODO(songshourui.null): Optimize the logic here. When executing
  // GreedyConstructElements, template_bundle().page_configs_ is not available
  // yet, so we need to check page_configs_ here. After optimization,
  // template_bundle().page_configs_ should be ready here. And we can directly
  // call template_bundle().ParallelConstructElement.
  if (page_configs_ && page_configs_->GetEnableParallelParseElementTemplate()) {
    template_bundle().GreedyConstructElements();
  }
  return true;
}

void LynxBinaryReader::RecordBinary() {
  template_bundle().SetBinary(
      std::vector<uint8_t>{stream_->begin(), stream_->end()});
}

LynxBinaryReader LynxBinaryReader::CreateLynxBinaryReader(
    std::vector<uint8_t> binary) {
  auto input_stream =
      std::make_unique<lynx::lepus::ByteArrayInputStream>(std::move(binary));
  auto reader = LynxBinaryReader{std::move(input_stream)};
  if (tasm::LynxEnv::GetInstance().IsDevToolEnabled()) {
    // record the original binary for debug if devtool is enabled
    reader.RecordBinary();
  }
  return reader;
}

StringListVec& LynxBinaryReader::GetStringList() {
  // use the string_list of template_bundle, so that there is no need to move it
  return template_bundle().GetStringList();
}

void LynxBinaryReader::SetStringList(const StringListVec& string_list) {
  template_bundle().string_list_ = string_list;
}

LynxTemplateBundle& LynxBinaryReader::template_bundle() {
  return template_bundle_;
}

LynxTemplateBundle LynxBinaryReader::GetTemplateBundle() {
  template_bundle().decode_start_timestamp_ = decode_start_timestamp_;
  template_bundle().decode_end_timestamp_ = decode_end_timestamp_;
  return std::move(template_bundle());
}

}  // namespace tasm
}  // namespace lynx
