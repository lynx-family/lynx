// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/template_bundle/template_codec/binary_decoder/template_binary_reader.h"

#include <chrono>
#include <map>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/template_assembler.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_lazy_reader_delegate.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "core/template_bundle/template_codec/template_binary.h"
#include "core/template_bundle/template_codec/ttml_constant.h"

namespace lynx {
namespace tasm {

TemplateBinaryReader::TemplateBinaryReader(
    PageConfigger* configger, TemplateEntry* entry,
    std::unique_ptr<lepus::InputStream> stream)
    : LynxBinaryReader(std::move(stream)),
      configger_(configger),
      entry_(entry) {
  enable_pre_process_attributes_ = false;
}

bool TemplateBinaryReader::DecodeCSSDescriptor() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeCSSDescriptor");

  // decode route
  ERROR_UNLESS(DecodeCSSDescriptorRoute());

  // check whether lazy-decode or async-decode is needed
  bool enable_css_async_decode = GetCSSAsyncDecode();
  bool enable_css_lazy_decode = enable_css_async_decode || GetCSSLazyDecode();

  auto& manager = template_bundle().GetCSSStyleManager();
  auto& target_fragments_map = *manager->GetCSSFragmentMap();

  if (!enable_css_lazy_decode) {
    ERROR_UNLESS(GreedyDecodeCSSDescriptor(target_fragments_map));
  }

  if (enable_css_async_decode) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeCSSDescriptorWithThread");
    const int length = css_section_range_.end - css_section_range_.start;
    auto css_reader = TemplateBinaryReader::Create(stream_->cursor(), length);
    css_reader->CopyForCSSAsyncDecode(*this);

    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        [css_reader = std::move(css_reader),
         manager = std::move(manager)]() mutable {
          css_reader->DecodeCSSFragmentAsync(std::move(manager));
        },
        base::ConcurrentTaskType::NORMAL_PRIORITY);
  }

  stream_->Seek(css_section_range_.end);
  return true;
}

std::unique_ptr<TemplateBinaryReader> TemplateBinaryReader::Create(
    const uint8_t* begin, size_t size) {
  auto binary_stream =
      std::make_unique<lepus::ByteArrayInputStream>(begin, size);
  auto reader = std::make_unique<TemplateBinaryReader>(
      nullptr, nullptr, std::move(binary_stream));
  return reader;
}

void TemplateBinaryReader::CopyForCSSAsyncDecode(
    const TemplateBinaryReader& other) {
  compile_options_ = other.compile_options_;
  enable_css_parser_ = other.enable_css_parser_;
  enable_css_variable_ = other.enable_css_variable_;
  enable_css_variable_multi_default_value_ =
      other.enable_css_variable_multi_default_value_;
  css_section_range_ = other.css_section_range_;
  lepus_chunk_route_ = other.lepus_chunk_route_;
}

bool TemplateBinaryReader::GetCSSLazyDecode() {
  if (compile_options_.enable_lazy_css_decode_ ==
      FeOption::FE_OPTION_UNDEFINED) {
    absetting_disable_css_lazy_decode_ = lynx::tasm::Config::GetConfigString(
        tasm::LynxEnv::Key::DISABLE_LAZY_CSS_DECODE, compile_options_);
    LOGI("CSSLazyDecode options FE_OPTION_UNDEFINED ABSetting: "
         << absetting_disable_css_lazy_decode_);
    if (config_decoder_) {
      config_decoder_->SetAbSettingDisableCSSLazyDecode(
          absetting_disable_css_lazy_decode_);
    }
    return !(absetting_disable_css_lazy_decode_ == "true");
  }
  LOGI("CSSLazyDecode options FE_OPTION: "
       << (int)compile_options_.enable_lazy_css_decode_);
  return compile_options_.enable_lazy_css_decode_ == FeOption::FE_OPTION_ENABLE;
}

bool TemplateBinaryReader::GetCSSAsyncDecode() {
  if (compile_options_.enable_async_css_decode_ == FeOption::FE_OPTION_ENABLE) {
    return true;
  }
  return false;
}

bool TemplateBinaryReader::DecodeCSSFragmentAsync(
    std::shared_ptr<CSSStyleSheetManager> manager) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeCSSFragmentAsync");
  auto& fragment_ranges = manager->route_.fragment_ranges;
  size_t descriptor_start = stream_->offset();
  for (auto it = fragment_ranges.begin(); it != fragment_ranges.end(); ++it) {
    if (manager->GetStopThread()) {
      break;
    }
    if (manager->IsSharedCSSFragmentDecoded(it->first)) {
      continue;
    }
    auto fragment = std::make_unique<SharedCSSFragment>(manager.get());
    stream_->Seek(descriptor_start + it->second.start);
    ERROR_UNLESS(
        DecodeCSSFragment(fragment.get(), it->second.end + descriptor_start));
    fragment->SetEnableClassMerge(compile_options_.enable_css_class_merge_);
    manager->AddSharedCSSFragment(std::move(fragment));
  }
  return true;
}

bool TemplateBinaryReader::DecodeCSSFragmentByIdInRender(int32_t id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LazyDecodeCSSFragment");
  const auto& manager = template_bundle().GetCSSStyleManager();
  auto& fragment_ranges = manager->route_.fragment_ranges;
  auto it = fragment_ranges.find(id);
  if (it == fragment_ranges.end()) {
    return false;
  }
  auto fragment = std::make_unique<SharedCSSFragment>(manager.get());
  stream_->Seek(css_section_range_.start + it->second.start);
  ERROR_UNLESS(DecodeCSSFragment(fragment.get(),
                                 it->second.end + css_section_range_.start));
  fragment->SetEnableClassMerge(compile_options_.enable_css_class_merge_);
  manager->AddSharedCSSFragment(std::move(fragment));
  return true;
}

bool TemplateBinaryReader::DidDecodeTemplate() {
  ERROR_UNLESS(LynxBinaryReader::DidDecodeTemplate());
  // when we construct a TemplateBinaryReader outside of loadTemplate
  // entry_ may be nullptr. Do nothing here.
  if (!entry_) {
    return true;
  }
  // different for LynxBinaryReader, TemplateBinaryReader need to init the
  // entry after decoding
  if (!entry_->InitWithPageConfigger(configger_)) {
    error_message_ = entry_->GetErrorMsg();
    return false;
  }

  return true;
}

std::shared_ptr<ElementTemplateInfo>
TemplateBinaryReader::DecodeElementTemplateInRender(const std::string& key) {
  return DecodeTemplatesInfoWithKey(key);
}

const std::shared_ptr<ParsedStyles>&
TemplateBinaryReader::GetParsedStylesInRender(const std::string& key) {
  return GetParsedStyles(key);
}

bool TemplateBinaryReader::DecodeContextBundleInRender(const std::string& key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LazyDecodeLepusChunk");
  const auto& iter = lepus_chunk_route_.start_offsets_.find(key);
  if (iter == lepus_chunk_route_.start_offsets_.end()) {
    return false;
  }
  std::shared_ptr<lepus::ContextBundle> bundle =
      lepus::ContextBundle::Create(compile_options_.enable_lepus_ng_);
  const auto& lepus_chunk_manager = template_bundle().GetLepusChunkManager();
  stream_->Seek(lepus_chunk_route_.descriptor_offset_ + iter->second);
  ERROR_UNLESS(DecodeContextBundle(bundle.get()));
  lepus_chunk_manager->AddLepusChunk(key, std::move(bundle));
  return true;
}

bool TemplateBinaryReader::DecodeParsedStylesSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeParsedStylesSection");
  // LazyDecode, only decode route.
  return DecodeParsedStylesRouter();
}

bool TemplateBinaryReader::DecodeElementTemplateSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeElementTemplateSection");
  // LazyDecode ElementTemplateSection, just exec DecodeElementTemplateRoute
  // when decode template.
  ERROR_UNLESS(DecodeElementTemplatesRouter());
  return true;
}

bool TemplateBinaryReader::DecodeLepusChunk() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeLepusChunk");
  ERROR_UNLESS(DecodeLepusChunkRoute());
  bool enable_lepus_chunk_async =
      compile_options_.enable_async_lepus_chunk_decode_;
  bool enable_lazy_decode =
      compile_options_.lynx_air_mode_ == CompileOptionAirMode::AIR_MODE_FIBER;
  if (enable_lepus_chunk_async) {
    // decode lepus chunk async
    const int length = lepus_chunk_range_.end - lepus_chunk_range_.start;
    auto lepus_chunk_reader =
        TemplateBinaryReader::Create(stream_->cursor(), length);
    lepus_chunk_reader->CopyForCSSAsyncDecode(*this);

    auto& lepus_chunk_manager = template_bundle().GetLepusChunkManager();
    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        [lepus_chunk_reader = std::move(lepus_chunk_reader),
         manager = lepus_chunk_manager]() mutable {
          lepus_chunk_reader->DecodeLepusChunkAsync(std::move(manager));
        },
        base::ConcurrentTaskType::NORMAL_PRIORITY);
  } else if (!enable_lazy_decode) {
    GreedyDecodeLepusChunk(
        template_bundle().GetLepusChunkManager()->lepus_chunk_map_);
  }
  stream_->Seek(lepus_chunk_range_.end);
  return true;
}

bool TemplateBinaryReader::DecodeLepusChunkAsync(
    std::shared_ptr<LepusChunkManager> manager) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeLepusChunkAsync");
  size_t descriptor_start = stream_->offset();
  auto& start_offsets = lepus_chunk_route_.start_offsets_;
  for (auto it = start_offsets.begin(); it != start_offsets.end(); ++it) {
    if (manager->GetStopThread()) {
      break;
    }
    if (manager->IsLepusChunkDecoded(it->first)) {
      continue;
    }
    stream_->Seek(descriptor_start + it->second);
    std::shared_ptr<lepus::ContextBundle> bundle =
        lepus::ContextBundle::Create(compile_options_.enable_lepus_ng_);
    ERROR_UNLESS(DecodeContextBundle(bundle.get()));
    manager->AddLepusChunk(it->first, std::move(bundle));
  }
  return true;
}

LynxTemplateBundle& TemplateBinaryReader::template_bundle() {
  // use the template bundle of entry directly, so that there is no need to move
  // the template_bundle
  return entry_ ? entry_->template_bundle()
                : LynxBinaryReader::template_bundle();
}

std::unique_ptr<LynxBinaryRecyclerDelegate>
TemplateBinaryReader::CreateRecycler() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CompleteDecode");
  // 0. copy the binary the template bundle
  auto recycler =
      TemplateBinaryReader::Create(stream_->begin(), stream_->size());
  recycler->template_bundle() = template_bundle();

  // 1. copy css settings
  recycler->CopyForCSSAsyncDecode(*this);
  recycler->enable_pre_process_attributes_ = true;

  // 2. copy parsed sytles rout
  recycler->string_key_parsed_styles_router_ = string_key_parsed_styles_router_;

  // 3. copy element template route
  recycler->element_templates_router_ = element_templates_router_;

  // 4. copy lepus chunk route
  recycler->lepus_chunk_route_ = lepus_chunk_route_;

  return recycler;
}

bool TemplateBinaryReader::CompleteDecode() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateTemplateBundleRecycler");

  // 0. decode css
  auto new_css_manager = std::make_shared<CSSStyleSheetManager>(nullptr);
  new_css_manager->SetEnableNewImportRule(GetEnableNewImportRule());
  ERROR_UNLESS(
      GreedyDecodeCSSDescriptor((*new_css_manager->GetCSSFragmentMap())));
  new_css_manager->FlattenAllCSSFragment();
  template_bundle().SetCSSStyleManager(std::move(new_css_manager));

  // 2. decode parsed styles
  ERROR_UNLESS(GreedyDecodeParsedStylesSection());

  // 3. decode element templates
  ERROR_UNLESS(GreedyDecodeElementTemplateSection());

  // 4. decode lepus chunk
  auto new_lepus_chunk_manager = std::make_shared<LepusChunkManager>();
  ERROR_UNLESS(
      GreedyDecodeLepusChunk(new_lepus_chunk_manager->lepus_chunk_map_));
  template_bundle().SetLepusChunkManager(std::move(new_lepus_chunk_manager));

  return true;
}

LynxTemplateBundle TemplateBinaryReader::GetCompleteTemplateBundle() {
  return std::move(template_bundle());
}

}  // namespace tasm
}  // namespace lynx
