// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"

#include <algorithm>
#include <string>
#include <vector>

#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/quick_context_pool.h"
#include "core/template_bundle/template_codec/template_binary.h"

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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DidDecodeTemplate");

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
  PrepareContext();
  return true;
}

Themed& LynxBinaryReader::Themed() { return template_bundle().themed_; }

bool LynxBinaryReader::DecodeCSSDescriptor() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeCSSDescriptor");
  // decode route
  ERROR_UNLESS(DecodeCSSDescriptorRoute());

  // greedy decode css section
  const auto& manager = template_bundle().GetCSSStyleManager();
  ERROR_UNLESS(GreedyDecodeCSSDescriptor((*manager->GetCSSFragmentMap())));

  // make all fragments read-only
  manager->FlattenAllCSSFragment();

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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeLepusChunk");
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeContext");
  auto& tb = template_bundle();
  tb.context_bundle_ = lepus::ContextBundle::Create(is_lepusng_binary_);

  ERROR_UNLESS(tb.context_bundle_);
  ERROR_UNLESS(DecodeContextBundle(tb.context_bundle_.get()));
  return true;
}

void LynxBinaryReader::PrepareContext() {
  // Contexts cannot be pre-created in two cases:
  // 1. not lepusNG
  // 2. will reuse context (dynamic component && no-diff)
  auto& tb = template_bundle();
  if (!tb.is_lepusng_binary() || tb.ShouldReuseLepusContext()) {
    return;
  }

  tb.quick_context_pool_ = lepus::QuickContextPool::Create(tb.context_bundle_);

  // if FE disables it in card, do not pre-create contexts. However, we reserve
  // the ability for the client to force pre-creation
  if (tb.page_configs_ && tb.page_configs_->GetEnableUseContextPool() &&
      !tb.page_configs_->GetDisableQuickTracingGC()) {
    constexpr int32_t kLocalQuickContextPoolSize = 1;
    tb.quick_context_pool_->FillPool(kLocalQuickContextPoolSize);
  }
}

bool LynxBinaryReader::DecodeParsedStylesSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeParsedStylesSection");
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeElementTemplateSection");
  ERROR_UNLESS(DecodeElementTemplatesRouter());
  ERROR_UNLESS(GreedyDecodeElementTemplateSection());
  return true;
}

bool LynxBinaryReader::DecodeCustomSectionsSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeCustomSections");
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
    ERROR_UNLESS(DecodeValue(&content, false));
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

std::vector<base::String>& LynxBinaryReader::string_list() {
  // use the string_list of template_bundle, so that there is no need to move it
  return template_bundle().string_list();
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
