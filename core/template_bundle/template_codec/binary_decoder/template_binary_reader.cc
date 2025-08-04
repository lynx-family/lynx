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
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/template_assembler.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/template_bundle/template_codec/binary_decoder/binary_decoder_trace_event_def.h"
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              TEMPLATE_BINARY_READER_DECODE_CSS_DESCRIPTOR);

  // decode route
  ERROR_UNLESS(DecodeCSSDescriptorRoute());

  // check whether lazy-decode or async-decode is needed
  bool enable_css_async_decode = GetCSSAsyncDecode();
  bool enable_css_lazy_decode = enable_css_async_decode || GetCSSLazyDecode();

  auto& manager = template_bundle().GetCSSStyleManager();

  if (page_configs_) {
    manager->SetFixCSSImportRuleOrder(
        page_configs_->GetFixCSSImportRuleOrder());
  }

  auto& target_fragments_map = *manager->GetCSSFragmentMap();

  if (!enable_css_lazy_decode) {
    ERROR_UNLESS(GreedyDecodeCSSDescriptor(target_fragments_map));
  }

  if (enable_css_async_decode) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                TEMPLATE_BINARY_READER_DECODE_CSS_DESCRIPTOR_WITH_THREAD);
    const int length = css_section_range_.end - css_section_range_.start;
    auto css_reader = TemplateBinaryReader::Create(stream_->cursor(), length);
    css_reader->CopyForAsyncDecode(*this);

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

/**
 * @brief Creates a StyleObjectDecoder instance.
 *
 * This function initializes a `lepus::ByteArrayInputStream` using the provided
 * data pointer and length, then creates a `LynxBinaryBaseCSSReader` instance
 * as a `StyleObjectDecoder` using the input stream.
 *
 * @param data Pointer to the start of the binary data for decoding.
 * @param length The length of the binary data.
 * @return A unique pointer to a `style::StyleObjectDecoder` instance.
 */
static std::unique_ptr<style::StyleObjectDecoder> StyleObjectDecoderCreator(
    uint8_t* data, size_t length, const StringListVec& string_list) {
  auto binary_stream =
      std::make_unique<lepus::ByteArrayInputStream>(data, length);
  auto* async_reader = new LynxBinaryBaseCSSReader(std::move(binary_stream));
  async_reader->SetStringList(string_list);
  return std::unique_ptr<style::StyleObjectDecoder>(async_reader);
}

/**
 * @brief Decodes style objects from the binary stream.
 *
 * If the simple styling feature is enabled, this function decodes the style
 * object route information, creates an array of style objects, and schedules
 * the decoding of these objects asynchronously on a concurrent thread. If
 * simple styling is not enabled, it skips the decoding process.
 *
 * @return `true` if the decoding process succeeds or is skipped, `false`
 * otherwise.
 */
bool TemplateBinaryReader::DecodeStyleObjects() {
  if (!compile_options_.enable_simple_styling_) {
    // simple styling is not enabled, skip this section
    return true;
  }
  DECODE_COMPACT_U32(section_count);
  // Decode style_object section
  if (section_count <
      static_cast<uint32_t>(StyleObjectSectionType::STYLE_OBJECT)) {
    return true;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeStyleObjectSection");
  StyleObjectRoute route;
  ERROR_UNLESS(DecodeStyleObjectRoute(route));
  size_t size = route.style_object_ranges.size();
  if (size > 0) {
    const auto& style_obj_array = template_bundle().InitStyleObjectList(size);
    const int length =
        style_objects_section_range_.end - style_objects_section_range_.start;
    auto cursor = stream_->cursor();
    size_t index = 0;
    auto* raw_style_obj_array = style_obj_array.get();
    for (auto i = route.style_object_ranges.begin();
         i != route.style_object_ranges.end() && index < size; ++i, index++) {
      auto* style_object_ref =
          new style::StyleObject(i->start, i->end, cursor, length,
                                 GetStringList(), StyleObjectDecoderCreator);
      raw_style_obj_array[index] = style_object_ref;
    }
    EnsureParallelParseTaskScheduler();
    task_schedular_->AsyncDecodeStyleObjects(style_obj_array);
    stream_->Seek(style_objects_section_range_.start +
                  route.style_object_ranges[size - 1].end);
  }
  // Decode keyframes section
  if (section_count <
      static_cast<uint32_t>(StyleObjectSectionType::STYLE_OBJECT_KEYFRAMES)) {
    return true;
  }
  StyleObjectRoute keyframes_route;
  ERROR_UNLESS(DecodeStyleObjectRoute(keyframes_route));
  size_t keyframe_size = keyframes_route.style_object_ranges.size();
  auto parser_config =
      CSSParserConfigs::GetCSSParserConfigsByComplierOptions(compile_options_);
  if (keyframe_size > 0) {
    auto keyframes = template_bundle().InitKeyframesMap(keyframe_size);
    for (const auto& range : keyframes_route.style_object_ranges) {
      stream_->Seek(style_objects_section_range_.start + range.start);
      DECODE_STDSTR(name);
      CSSKeyframesToken* token = new CSSKeyframesToken(parser_config);
      ERROR_UNLESS(DecodeCSSKeyframesToken(token));
      keyframes.emplace(std::move(name), token);
      stream_->Seek(style_objects_section_range_.start + range.end);
    }
    stream_->Seek(style_objects_section_range_.start +
                  keyframes_route.style_object_ranges[keyframe_size - 1].end);
  }
  // Decode other sub_section below
  stream_->Seek(style_objects_section_range_.end);
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

void TemplateBinaryReader::CopyForAsyncDecode(TemplateBinaryReader& other) {
  compile_options_ = other.compile_options_;
  enable_css_parser_ = other.enable_css_parser_;
  enable_css_variable_ = other.enable_css_variable_;
  enable_css_variable_multi_default_value_ =
      other.enable_css_variable_multi_default_value_;
  css_section_range_ = other.css_section_range_;
  lepus_chunk_route_ = other.lepus_chunk_route_;
  template_bundle().SetStringList(other.GetStringList());
}

void TemplateBinaryReader::EnsureParallelParseTaskScheduler() {
  if (task_schedular_ == nullptr) {
    task_schedular_ = std::make_unique<ParallelParseTaskScheduler>();
  }
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              TEMPLATE_BINARY_READER_DECODE_CSS_FRAGMENT_ASYNC);
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              TEMPLATE_BINARY_READER_LAZY_DECODE_CSS_FRAGMENT);
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              TEMPLATE_BINARY_READER_LAZY_DECODE_LEPUS_CHUNK);
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              TEMPLATE_BINARY_READER_DECODE_PARSED_STYLES_SECTION);
  // LazyDecode, only decode route.
  return DecodeParsedStylesRouter();
}

bool TemplateBinaryReader::DecodeElementTemplateSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              TEMPLATE_BINARY_READER_DECODE_ELEMENT_TEMPLATE_SECTION);
  // LazyDecode ElementTemplateSection, just exec DecodeElementTemplateRoute
  // when decode template.
  ERROR_UNLESS(DecodeElementTemplatesRouter());
  if (page_configs_ && page_configs_->GetEnableParallelParseElementTemplate()) {
    ERROR_UNLESS(ParallelDecodeElementTemplate());
  }
  return true;
}

bool TemplateBinaryReader::ParallelDecodeElementTemplate() {
  EnsureParallelParseTaskScheduler();
  return task_schedular_->ParallelParseElementTemplate(
      &element_templates_router_, this);
}

ElementTemplateResult TemplateBinaryReader::GetElementTemplateParseResult(
    const std::string& key) {
  EnsureParallelParseTaskScheduler();
  return task_schedular_->TryGetElementTemplateParseResult(key);
}

bool TemplateBinaryReader::DecodeLepusChunk() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_BINARY_READER_DECODE_LEPUS_CHUNK);
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
    lepus_chunk_reader->CopyForAsyncDecode(*this);

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
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              TEMPLATE_BINARY_READER_DECODE_LEPUS_CHUNK_ASYNC);
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_BINARY_READER_COMPLETE_DECODE);
  // 0. copy the binary the template bundle
  auto recycler =
      TemplateBinaryReader::Create(stream_->begin(), stream_->size());
  recycler->template_bundle() = template_bundle();

  // 1. copy css settings
  recycler->CopyForAsyncDecode(*this);
  recycler->enable_pre_process_attributes_ = true;

  // 2. copy parsed styles route
  recycler->string_key_parsed_styles_router_ = string_key_parsed_styles_router_;

  // 3. copy element template route
  recycler->element_templates_router_ = element_templates_router_;

  // 4. copy lepus chunk route
  recycler->lepus_chunk_route_ = lepus_chunk_route_;

  return recycler;
}

bool TemplateBinaryReader::CompleteDecode() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_BINARY_READER_CREATE_RECYCLER);

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
